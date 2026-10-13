module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, exports = {}, format_version = 8 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}, {default = 0 : i64, name = "immediate", type = "u32"}, {default = false, name = "immediate_rhs", type = "bool"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "lhs", type = "u32"}, {default = 0 : i64, name = "rhs", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = false, name = "writes", type = "bool"}]}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.decode_stage.out", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.decode_stage.out", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.execute.out", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.execute.out", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.fetch.out", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.fetch.out", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.retirement", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.retirement", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}], schedule = [4, 3, 2, 1, 0], stage = "rtl"} {
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
    %28 = acir.current {aspect = "count", resource = "top.fetch.out"} : i32 loc(#loc6)
    %29 = acir.current {aspect = "count", resource = "top.retirement"} : i32 loc(#loc7)
    %c0_i32 = arith.constant 0 : i32 loc(#loc7)
    %30 = arith.cmpi eq, %29, %c0_i32 : i32 loc(#loc7)
    %31 = arith.xori %30, %true_0 : i1 loc(#loc7)
    %false_1 = arith.constant {acir.source_type = "bool"} false
    %32 = acir.current {aspect = "value", resource = "top.rf.pending"} : !acir.array<i1, 32> loc(#loc8)
    %33 = acir.current {aspect = "storage", resource = "top.fetch.out"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc9)
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
    %109 = arith.xori %31, %true_0 : i1
    %110 = arith.ori %109, %true : i1
    %111 = arith.andi %31, %110 : i1
    %112 = arith.xori %30, %true_0 : i1
    %113 = arith.ori %112, %true : i1
    %114 = arith.xori %30, %true_0 : i1
    %115 = arith.xori %30, %true_0 : i1
    %116 = arith.xori %30, %true_0 : i1 loc(#loc22)
    %117 = arith.ori %116, %113 : i1 loc(#loc22)
    %118 = arith.ori %115, %117 : i1 loc(#loc22)
    %119 = arith.ori %114, %118 : i1 loc(#loc22)
    %120 = arith.xori %96, %true_0 : i1
    %121 = arith.xori %90, %true_0 : i1
    %122 = arith.andi %35, %35 : i1 loc(#loc23)
    %123 = arith.xori %40, %true_0 : i1
    %124 = arith.xori %40, %true_0 : i1
    %125 = arith.ori %124, %35 : i1
    %126 = arith.ori %123, %125 : i1
    %127 = arith.xori %43, %true_0 : i1
    %128 = arith.xori %43, %true_0 : i1
    %129 = arith.andi %125, %35 : i1
    %130 = arith.ori %128, %129 : i1
    %131 = arith.ori %127, %130 : i1
    %132 = arith.andi %131, %35 : i1
    %133 = arith.andi %43, %132 : i1
    %134 = arith.andi %40, %41 : i1
    %135 = arith.xori %134, %true_0 : i1
    %136 = arith.xori %134, %true_0 : i1
    %137 = arith.andi %125, %35 : i1
    %138 = arith.ori %136, %137 : i1
    %139 = arith.ori %135, %138 : i1
    %140 = arith.andi %139, %35 : i1
    %141 = arith.andi %134, %140 : i1
    %142 = arith.ori %133, %141 : i1
    %143 = arith.andi %126, %142 : i1
    %144 = arith.andi %40, %143 : i1
    %145 = arith.xori %39, %true_0 : i1
    %146 = arith.ori %145, %35 : i1
    %147 = arith.andi %39, %146 : i1
    %148 = arith.ori %144, %147 : i1
    %149 = arith.andi %122, %148 : i1
    %150 = arith.andi %35, %35 : i1
    %151 = arith.andi %131, %150 : i1
    %152 = arith.andi %43, %151 : i1
    %153 = arith.andi %139, %35 : i1
    %154 = arith.andi %134, %153 : i1
    %155 = arith.ori %152, %154 : i1
    %156 = arith.andi %126, %155 : i1
    %157 = arith.andi %40, %156 : i1
    %158 = arith.andi %146, %35 : i1
    %159 = arith.andi %39, %158 : i1
    %160 = arith.ori %157, %159 : i1
    %161 = arith.andi %149, %160 : i1
    %162 = arith.andi %161, %35 : i1
    %163 = arith.andi %35, %35 : i1
    %164 = arith.andi %131, %163 : i1
    %165 = arith.andi %43, %164 : i1
    %166 = arith.andi %139, %35 : i1
    %167 = arith.andi %134, %166 : i1
    %168 = arith.ori %165, %167 : i1
    %169 = arith.andi %126, %168 : i1
    %170 = arith.andi %40, %169 : i1
    %171 = arith.andi %39, %146 : i1
    %172 = arith.ori %170, %171 : i1
    %173 = arith.andi %162, %172 : i1
    %174 = arith.xori %64, %true_0 : i1
    %175 = arith.xori %64, %true_0 : i1
    %176 = arith.andi %130, %35 : i1
    %177 = arith.ori %175, %176 : i1
    %178 = arith.ori %174, %177 : i1
    %179 = arith.xori %64, %true_0 : i1 loc(#loc24)
    %180 = arith.ori %179, %178 : i1 loc(#loc24)
    %181 = arith.andi %35, %35 : i1 loc(#loc14)
    %182 = arith.andi %178, %181 : i1 loc(#loc14)
    %183 = arith.andi %64, %182 : i1 loc(#loc14)
    %184 = arith.andi %180, %183 : i1 loc(#loc14)
    %185 = arith.andi %64, %184 : i1 loc(#loc14)
    %186 = arith.andi %178, %185 : i1 loc(#loc14)
    %187 = arith.andi %64, %186 : i1 loc(#loc14)
    %188 = arith.andi %43, %44 : i1
    %189 = arith.xori %188, %true_0 : i1
    %190 = arith.andi %130, %35 : i1
    %191 = arith.ori %189, %190 : i1
    %192 = arith.xori %188, %true_0 : i1 loc(#loc25)
    %193 = arith.ori %192, %191 : i1 loc(#loc25)
    %194 = arith.andi %191, %35 : i1 loc(#loc25)
    %195 = arith.andi %188, %194 : i1 loc(#loc25)
    %196 = arith.andi %193, %195 : i1 loc(#loc25)
    %197 = arith.andi %188, %196 : i1 loc(#loc25)
    %198 = arith.andi %191, %197 : i1 loc(#loc25)
    %199 = arith.andi %188, %198 : i1 loc(#loc25)
    %200 = arith.ori %187, %199 : i1 loc(#loc25)
    %201 = arith.andi %131, %200 : i1 loc(#loc25)
    %202 = arith.andi %43, %201 : i1 loc(#loc25)
    %203 = arith.andi %134, %139 : i1
    %204 = arith.ori %202, %203 : i1
    %205 = arith.andi %126, %204 : i1
    %206 = arith.andi %40, %205 : i1
    %207 = arith.xori %39, %true_0 : i1 loc(#loc26)
    %208 = arith.ori %207, %146 : i1 loc(#loc26)
    %209 = arith.andi %146, %35 : i1 loc(#loc26)
    %210 = arith.andi %39, %209 : i1 loc(#loc26)
    %211 = arith.andi %208, %210 : i1 loc(#loc26)
    %212 = arith.andi %39, %211 : i1 loc(#loc26)
    %213 = arith.andi %146, %212 : i1 loc(#loc26)
    %214 = arith.andi %39, %213 : i1 loc(#loc26)
    %215 = arith.ori %206, %214 : i1 loc(#loc26)
    %216 = arith.andi %173, %215 : i1 loc(#loc26)
    %217 = arith.andi %126, %35 : i1
    %218 = arith.andi %40, %217 : i1
    %219 = arith.andi %39, %146 : i1
    %220 = arith.ori %218, %219 : i1
    %221 = arith.andi %216, %220 : i1
    %222 = arith.andi %131, %35 : i1
    %223 = arith.andi %43, %222 : i1
    %224 = arith.andi %134, %139 : i1
    %225 = arith.ori %223, %224 : i1
    %226 = arith.andi %126, %225 : i1
    %227 = arith.andi %40, %226 : i1
    %228 = arith.andi %39, %146 : i1
    %229 = arith.ori %227, %228 : i1
    %230 = arith.andi %221, %229 : i1
    %231 = arith.xori %64, %true_0 : i1 loc(#loc27)
    %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
    %232 = arith.cmpi eq, %38, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32
    %233 = arith.xori %232, %true_0 : i1
    %234 = arith.andi %64, %233 : i1
    %235 = arith.xori %234, %true_0 : i1
    %236 = arith.andi %177, %35 : i1
    %237 = arith.ori %235, %236 : i1
    %238 = arith.andi %234, %237 : i1
    %239 = arith.andi %64, %232 : i1
    %240 = arith.xori %239, %true_0 : i1
    %241 = arith.andi %177, %35 : i1
    %242 = arith.ori %240, %241 : i1
    %243 = arith.andi %242, %35 : i1
    %244 = arith.andi %239, %243 : i1
    %245 = arith.ori %238, %244 : i1
    %false_10 = arith.constant {acir.source_type = "bool"} false
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32 loc(#loc28)
    %246 = arith.shrui %37, %c12_i32 : i32 loc(#loc28)
    %247 = arith.andi %246, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc28)
    %248 = arith.cmpi eq, %247, %c2_i32 {acir.id = "v92", acir.source_type = "bool"} : i32 loc(#loc28)
    %249 = arith.select %234, %false_10, %248 : i1 loc(#loc28)
    %250 = acir.assert %249 loc(#loc28)
    %251 = arith.andi %245, %250 : i1 loc(#loc28)
    %252 = arith.ori %231, %251 : i1 loc(#loc28)
    %253 = arith.xori %188, %true_0 : i1 loc(#loc29)
    %254 = arith.cmpi eq, %247, %c2_i32 {acir.id = "v80", acir.source_type = "bool"} : i32 loc(#loc30)
    %255 = acir.assert %254 loc(#loc30)
    %256 = arith.andi %35, %255 : i1 loc(#loc30)
    %257 = arith.ori %253, %256 : i1 loc(#loc30)
    %258 = arith.andi %252, %257 : i1 loc(#loc30)
    %259 = arith.xori %134, %true_0 : i1 loc(#loc31)
    %260 = arith.cmpi eq, %247, %c0_i32_4 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc32)
    %261 = arith.xori %260, %true_0 : i1 loc(#loc32)
    %262 = arith.andi %134, %261 : i1 loc(#loc32)
    %263 = arith.xori %262, %true_0 : i1
    %264 = arith.andi %138, %35 : i1
    %265 = arith.ori %263, %264 : i1
    %266 = arith.andi %262, %265 : i1
    %267 = arith.andi %134, %260 : i1
    %268 = arith.xori %267, %true_0 : i1
    %269 = arith.xori %267, %true_0 : i1
    %270 = arith.andi %138, %35 : i1
    %271 = arith.ori %269, %270 : i1
    %272 = arith.ori %268, %271 : i1
    %273 = arith.cmpi eq, %46, %c0_i32_4 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc33)
    %274 = arith.xori %273, %true_0 : i1 loc(#loc33)
    %275 = arith.andi %267, %274 : i1 loc(#loc33)
    %276 = arith.xori %275, %true_0 : i1
    %277 = arith.andi %271, %35 : i1
    %278 = arith.ori %276, %277 : i1
    %279 = arith.andi %278, %35 : i1
    %280 = arith.andi %275, %279 : i1
    %281 = arith.andi %267, %273 : i1
    %282 = arith.xori %281, %true_0 : i1
    %283 = arith.andi %271, %35 : i1
    %284 = arith.ori %282, %283 : i1
    %285 = arith.andi %281, %284 : i1
    %286 = arith.ori %280, %285 : i1
    %287 = arith.andi %272, %286 : i1
    %288 = arith.andi %267, %287 : i1
    %289 = arith.ori %266, %288 : i1
    %290 = arith.cmpi eq, %46, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32 loc(#loc34)
    %291 = arith.select %275, %290, %true_7 : i1 loc(#loc34)
    %292 = arith.select %262, %false_10, %291 : i1 loc(#loc34)
    %293 = acir.assert %292 loc(#loc34)
    %294 = arith.andi %289, %293 : i1 loc(#loc34)
    %295 = arith.ori %259, %294 : i1 loc(#loc34)
    %296 = arith.andi %258, %295 : i1 loc(#loc34)
    %297 = arith.xori %39, %true_0 : i1 loc(#loc35)
    %298 = arith.cmpi eq, %247, %c0_i32_4 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc36)
    %299 = acir.assert %298 loc(#loc36)
    %300 = arith.andi %35, %299 : i1 loc(#loc36)
    %301 = arith.ori %297, %300 : i1 loc(#loc36)
    %302 = arith.andi %296, %301 : i1 loc(#loc36)
    %303 = arith.andi %230, %302 : i1 loc(#loc36)
    %304 = arith.andi %113, %303 : i1 loc(#loc36)
    %305 = arith.andi %30, %304 : i1 loc(#loc36)
    %306 = arith.andi %true, %305 : i1 loc(#loc36)
    %307 = arith.andi %117, %306 : i1 loc(#loc36)
    %308 = arith.ori %121, %307 : i1 loc(#loc36)
    %309 = arith.andi %90, %308 : i1 loc(#loc36)
    %310 = arith.andi %30, %88 : i1
    %311 = arith.xori %310, %true_0 : i1
    %312 = arith.andi %117, %306 : i1
    %313 = arith.ori %311, %312 : i1
    %314 = arith.andi %true, %305 : i1 loc(#loc37)
    %315 = arith.andi %313, %314 : i1 loc(#loc37)
    %316 = arith.andi %310, %315 : i1 loc(#loc37)
    %317 = arith.ori %309, %316 : i1 loc(#loc37)
    %318 = arith.andi %118, %317 : i1 loc(#loc37)
    %319 = arith.ori %120, %318 : i1 loc(#loc37)
    %320 = arith.andi %96, %319 : i1 loc(#loc37)
    %321 = arith.xori %97, %true_0 : i1
    %322 = arith.xori %97, %true_0 : i1
    %323 = arith.andi %118, %317 : i1
    %324 = arith.ori %322, %323 : i1
    %325 = arith.ori %321, %324 : i1
    %326 = arith.xori %101, %true_0 : i1
    %327 = arith.andi %324, %305 : i1
    %328 = arith.ori %326, %327 : i1
    %329 = arith.andi %true, %305 : i1 loc(#loc38)
    %330 = arith.andi %328, %329 : i1 loc(#loc38)
    %331 = arith.andi %101, %330 : i1 loc(#loc38)
    %332 = arith.andi %97, %99 : i1
    %333 = arith.xori %332, %true_0 : i1
    %334 = arith.andi %324, %305 : i1
    %335 = arith.ori %333, %334 : i1
    %336 = arith.andi %332, %335 : i1
    %337 = arith.ori %331, %336 : i1
    %338 = arith.andi %325, %337 : i1
    %339 = arith.andi %97, %338 : i1
    %340 = arith.ori %320, %339 : i1
    %341 = arith.andi %119, %340 : i1
    %342 = arith.andi %30, %341 : i1
    %343 = arith.andi %113, %342 : i1
    %344 = arith.andi %30, %343 : i1
    %345 = arith.ori %111, %344 : i1
    %346 = arith.ori %108, %345 : i1
    %347 = arith.andi %346, %303 : i1
    %348 = arith.andi %107, %347 : i1
    %349 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %350 = arith.xori %107, %true_0 : i1 loc(#loc40)
    %351 = arith.ori %350, %346 : i1 loc(#loc40)
    %c0_i32_11 = arith.constant 0 : i32 loc(#loc39)
    %352 = acir.extract %33[%c0_i32_11] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc39)
    %c0_i32_12 = arith.constant 0 : i32 loc(#loc39)
    %353 = arith.cmpi ne, %28, %c0_i32_12 : i32 loc(#loc39)
    %354 = arith.andi %351, %353 : i1 loc(#loc39)
    %355 = arith.ori %349, %354 : i1 loc(#loc39)
    %356 = arith.andi %348, %355 : i1 loc(#loc39)
    %357 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %358 = arith.andi %351, %353 : i1 loc(#loc39)
    %359 = arith.ori %357, %358 : i1 loc(#loc39)
    %360 = arith.andi %356, %359 : i1 loc(#loc39)
    %361 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %c0_i32_13 = arith.constant 0 : i32 loc(#loc39)
    %362 = arith.cmpi eq, %28, %c0_i32_13 : i32 loc(#loc39)
    %363 = arith.andi %351, %true : i1 loc(#loc39)
    %364 = arith.ori %361, %363 : i1 loc(#loc39)
    %365 = arith.andi %360, %364 : i1 loc(#loc39)
    %366 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %367 = arith.xori %362, %true_0 : i1 loc(#loc39)
    %368 = arith.andi %351, %367 : i1 loc(#loc39)
    %369 = arith.ori %366, %368 : i1 loc(#loc39)
    %370 = arith.andi %365, %369 : i1 loc(#loc39)
    %371 = arith.xori %107, %true_0 : i1
    %372 = arith.xori %107, %true_0 : i1 loc(#loc41)
    %373 = arith.ori %372, %351 : i1 loc(#loc41)
    %374 = arith.andi %346, %303 : i1 loc(#loc40)
    %375 = arith.andi %107, %374 : i1 loc(#loc40)
    %376 = arith.andi %351, %375 : i1 loc(#loc40)
    %377 = arith.andi %107, %376 : i1 loc(#loc40)
    %378 = arith.andi %373, %377 : i1 loc(#loc40)
    %379 = arith.ori %371, %378 : i1 loc(#loc40)
    %380 = arith.andi %370, %379 : i1 loc(#loc40)
    %381 = acir.extract %85 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc42)
    %c0_i32_14 = arith.constant {acir.source_type = "u32"} 0 : i32
    %382 = arith.cmpi eq, %381, %c0_i32_14 {acir.id = "v5", acir.source_type = "bool"} : i32
    %383 = arith.xori %382, %true_0 : i1
    %384 = arith.andi %107, %383 : i1
    %385 = arith.xori %384, %true_0 : i1
    %386 = arith.xori %384, %true_0 : i1
    %387 = arith.andi %373, %377 : i1
    %388 = arith.ori %386, %387 : i1
    %389 = acir.current {aspect = "value", resource = "top.rf.registers"} : !acir.array<i32, 32> loc(#loc43)
    %390 = arith.andi %388, %true : i1 loc(#loc43)
    %391 = arith.ori %385, %390 : i1 loc(#loc43)
    %392 = arith.andi %380, %391 : i1 loc(#loc43)
    %393 = arith.xori %107, %true_0 : i1
    %394 = arith.xori %107, %true_0 : i1 loc(#loc44)
    %395 = arith.ori %394, %351 : i1 loc(#loc44)
    %396 = arith.andi %351, %375 : i1 loc(#loc44)
    %397 = arith.andi %107, %396 : i1 loc(#loc44)
    %398 = arith.andi %395, %397 : i1 loc(#loc44)
    %399 = arith.ori %393, %398 : i1 loc(#loc44)
    %400 = arith.andi %392, %399 : i1 loc(#loc44)
    %401 = acir.extract %85 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc45)
    %c0_i32_15 = arith.constant {acir.source_type = "u32"} 0 : i32
    %402 = arith.cmpi eq, %401, %c0_i32_15 {acir.id = "v5", acir.source_type = "bool"} : i32
    %403 = arith.xori %402, %true_0 : i1
    %404 = arith.andi %107, %403 : i1
    %405 = arith.xori %404, %true_0 : i1
    %406 = arith.xori %404, %true_0 : i1
    %407 = arith.andi %395, %397 : i1
    %408 = arith.ori %406, %407 : i1
    %409 = arith.andi %408, %true : i1 loc(#loc43)
    %410 = arith.ori %405, %409 : i1 loc(#loc43)
    %411 = arith.andi %400, %410 : i1 loc(#loc43)
    %412 = arith.xori %107, %true_0 : i1
    %413 = arith.andi %351, %375 : i1
    %414 = arith.ori %412, %413 : i1
    %415 = arith.andi %411, %414 : i1
    %416 = arith.xori %107, %true_0 : i1
    %417 = arith.xori %107, %true_0 : i1 loc(#loc46)
    %418 = arith.xori %107, %true_0 : i1
    %419 = arith.ori %418, %351 : i1
    %420 = arith.ori %417, %419 : i1
    %421 = arith.andi %419, %375 : i1 loc(#loc46)
    %422 = arith.andi %107, %421 : i1 loc(#loc46)
    %423 = arith.andi %420, %422 : i1 loc(#loc46)
    %424 = arith.ori %416, %423 : i1 loc(#loc46)
    %425 = arith.andi %415, %424 : i1 loc(#loc46)
    %426 = arith.xori %107, %true_0 : i1
    %427 = arith.xori %107, %true_0 : i1
    %428 = arith.ori %427, %420 : i1
    %429 = acir.extract %85 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc47)
    %430 = arith.xori %429, %true_0 : i1 loc(#loc47)
    %431 = arith.andi %107, %430 : i1 loc(#loc47)
    %432 = arith.xori %431, %true_0 : i1
    %433 = arith.andi %420, %422 : i1
    %434 = arith.ori %432, %433 : i1
    %435 = arith.andi %431, %434 : i1
    %436 = arith.andi %107, %429 : i1
    %437 = arith.xori %436, %true_0 : i1
    %438 = arith.andi %420, %422 : i1
    %439 = arith.ori %437, %438 : i1
    %440 = arith.andi %439, %422 : i1
    %441 = arith.andi %436, %440 : i1
    %442 = arith.ori %435, %441 : i1
    %443 = arith.andi %428, %442 : i1
    %444 = arith.ori %426, %443 : i1
    %445 = arith.andi %425, %444 : i1
    %false_16 = arith.constant {acir.source_type = "bool"} false
    %446 = acir.extract %85 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc48)
    %c0_i32_17 = arith.constant {acir.source_type = "u32"} 0 : i32
    %447 = arith.cmpi ne, %446, %c0_i32_17 {acir.id = "v8", acir.source_type = "bool"} : i32
    %448 = arith.select %431, %false_16, %447 : i1
    %449 = arith.andi %107, %448 : i1
    %450 = arith.xori %449, %true_0 : i1
    %451 = arith.xori %449, %true_0 : i1
    %452 = arith.andi %428, %442 : i1
    %453 = arith.ori %451, %452 : i1
    %454 = arith.andi %453, %true : i1 loc(#loc49)
    %455 = arith.ori %450, %454 : i1 loc(#loc49)
    %456 = arith.andi %445, %455 : i1 loc(#loc49)
    %457 = arith.xori %449, %true_0 : i1 loc(#loc49)
    %458 = arith.andi %453, %true : i1 loc(#loc49)
    %459 = arith.ori %457, %458 : i1 loc(#loc49)
    %460 = arith.andi %456, %459 : i1 loc(#loc49)
    %461 = arith.xori %449, %true_0 : i1 loc(#loc49)
    %462 = arith.andi %true, %422 : i1 loc(#loc49)
    %463 = arith.andi %453, %462 : i1 loc(#loc49)
    %464 = arith.ori %461, %463 : i1 loc(#loc49)
    %465 = arith.andi %460, %464 : i1 loc(#loc49)
    %466 = arith.andi %375, %375 : i1 loc(#loc50)
    %467 = arith.andi %466, %375 : i1 loc(#loc50)
    %468 = arith.andi %467, %375 : i1 loc(#loc50)
    %469 = arith.andi %387, %377 : i1
    %470 = arith.andi %384, %469 : i1
    %471 = arith.andi %true, %470 : i1
    %472 = arith.andi %388, %471 : i1
    %473 = arith.andi %384, %472 : i1
    %474 = arith.andi %107, %382 : i1
    %475 = arith.xori %474, %true_0 : i1 loc(#loc51)
    %476 = arith.andi %373, %377 : i1
    %477 = arith.ori %475, %476 : i1
    %478 = arith.andi %474, %477 : i1
    %479 = arith.ori %473, %478 : i1
    %480 = arith.andi %468, %479 : i1
    %481 = acir.extract %85 {acir.id = "v10", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc52)
    %482 = arith.xori %481, %true_0 : i1 loc(#loc52)
    %483 = arith.andi %107, %482 : i1 loc(#loc52)
    %484 = arith.xori %483, %true_0 : i1
    %485 = arith.andi %351, %375 : i1
    %486 = arith.ori %484, %485 : i1
    %487 = arith.andi %407, %397 : i1
    %488 = arith.andi %404, %487 : i1
    %489 = arith.andi %true, %488 : i1
    %490 = arith.andi %408, %489 : i1
    %491 = arith.andi %404, %490 : i1
    %492 = arith.andi %107, %402 : i1
    %493 = arith.xori %492, %true_0 : i1 loc(#loc51)
    %494 = arith.andi %395, %397 : i1
    %495 = arith.ori %493, %494 : i1
    %496 = arith.andi %492, %495 : i1
    %497 = arith.ori %491, %496 : i1
    %498 = arith.andi %486, %497 : i1
    %499 = arith.andi %483, %498 : i1
    %500 = arith.andi %107, %481 : i1
    %501 = arith.xori %500, %true_0 : i1
    %502 = arith.andi %351, %375 : i1
    %503 = arith.ori %501, %502 : i1
    %504 = arith.andi %503, %375 : i1
    %505 = arith.andi %500, %504 : i1
    %506 = arith.ori %499, %505 : i1
    %507 = arith.andi %480, %506 : i1
    %508 = arith.andi %507, %497 : i1
    %509 = arith.andi %508, %375 : i1
    %510 = arith.andi %419, %509 : i1
    %511 = arith.andi %107, %510 : i1
    %512 = arith.andi %107, %511 : i1
    %513 = arith.xori %512, %true_0 : i1
    %514 = arith.ori %513, %346 : i1
    %515 = arith.andi %465, %514 : i1
    %516 = arith.xori %512, %true_0 : i1 loc(#loc53)
    %517 = acir.current {aspect = "count", resource = "top.decode_stage.out"} : i32 loc(#loc53)
    %518 = acir.current {aspect = "storage", resource = "top.decode_stage.out"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc54)
    %c0_i32_18 = arith.constant 0 : i32 loc(#loc54)
    %519 = acir.extract %518[%c0_i32_18] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc54)
    %c0_i32_19 = arith.constant 0 : i32 loc(#loc54)
    %520 = arith.cmpi ne, %517, %c0_i32_19 : i32 loc(#loc54)
    %521 = arith.andi %520, %520 : i1 loc(#loc54)
    %c0_i32_20 = arith.constant 0 : i32 loc(#loc54)
    %522 = arith.cmpi eq, %517, %c0_i32_20 : i32 loc(#loc54)
    %523 = arith.andi %521, %true : i1 loc(#loc54)
    %524 = arith.xori %522, %true_0 : i1 loc(#loc54)
    %525 = arith.andi %523, %524 : i1 loc(#loc54)
    %526 = arith.andi %525, %520 : i1 loc(#loc54)
    %527 = arith.andi %520, %520 : i1 loc(#loc55)
    %528 = arith.andi %527, %520 : i1 loc(#loc55)
    %529 = arith.andi %528, %520 : i1 loc(#loc55)
    %530 = acir.extract %519 {acir.id = "v5", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc56)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc57)
    %531 = arith.cmpi eq, %530, %c1_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc57)
    %532 = arith.xori %531, %true_0 : i1 loc(#loc57)
    %533 = arith.xori %532, %true_0 : i1
    %534 = arith.ori %533, %520 : i1
    %535 = arith.andi %520, %520 : i1 loc(#loc58)
    %536 = arith.andi %534, %535 : i1 loc(#loc58)
    %537 = arith.andi %532, %536 : i1 loc(#loc58)
    %538 = arith.xori %531, %true_0 : i1
    %539 = arith.ori %538, %520 : i1
    %540 = arith.andi %520, %520 : i1 loc(#loc59)
    %541 = arith.andi %539, %540 : i1 loc(#loc59)
    %542 = arith.andi %531, %541 : i1 loc(#loc59)
    %543 = arith.ori %537, %542 : i1 loc(#loc59)
    %544 = arith.andi %529, %543 : i1 loc(#loc59)
    %545 = arith.andi %544, %520 : i1 loc(#loc59)
    %546 = arith.andi %545, %520 : i1 loc(#loc59)
    %547 = arith.xori %546, %true_0 : i1 loc(#loc59)
    %548 = acir.current {aspect = "count", resource = "top.execute.out"} : i32 loc(#loc60)
    %549 = acir.current {aspect = "storage", resource = "top.execute.out"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc61)
    %c0_i32_21 = arith.constant 0 : i32 loc(#loc61)
    %550 = acir.extract %549[%c0_i32_21] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc61)
    %c0_i32_22 = arith.constant 0 : i32 loc(#loc61)
    %551 = arith.cmpi ne, %548, %c0_i32_22 : i32 loc(#loc61)
    %552 = arith.andi %551, %551 : i1 loc(#loc61)
    %c0_i32_23 = arith.constant 0 : i32 loc(#loc61)
    %553 = arith.cmpi eq, %548, %c0_i32_23 : i32 loc(#loc61)
    %554 = arith.andi %552, %true : i1 loc(#loc61)
    %555 = arith.xori %553, %true_0 : i1 loc(#loc61)
    %556 = arith.andi %554, %555 : i1 loc(#loc61)
    %557 = arith.andi %556, %551 : i1 loc(#loc61)
    %558 = acir.extract %550 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc62)
    %c2_i32_24 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc63)
    %559 = arith.cmpi eq, %558, %c2_i32_24 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc63)
    %560 = arith.xori %559, %true_0 : i1 loc(#loc63)
    %561 = arith.xori %560, %true_0 : i1 loc(#loc63)
    %562 = arith.xori %560, %true_0 : i1
    %563 = arith.ori %562, %551 : i1
    %564 = arith.andi %563, %551 : i1
    %565 = arith.ori %561, %564 : i1
    %566 = arith.andi %557, %565 : i1
    %567 = acir.extract %550 {acir.id = "v12", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc64)
    %c3_i32_25 = arith.constant {acir.source_type = "u32"} 3 : i32
    %568 = arith.cmpi eq, %567, %c3_i32_25 {acir.id = "v14", acir.source_type = "bool"} : i32
    %569 = arith.andi %560, %568 : i1
    %570 = arith.xori %569, %true_0 : i1
    %571 = arith.xori %569, %true_0 : i1 loc(#loc65)
    %572 = arith.xori %569, %true_0 : i1
    %573 = arith.andi %563, %551 : i1
    %574 = arith.ori %572, %573 : i1
    %575 = arith.ori %571, %574 : i1
    %576 = arith.andi %574, %551 : i1 loc(#loc65)
    %577 = arith.andi %569, %576 : i1 loc(#loc65)
    %578 = arith.andi %575, %577 : i1 loc(#loc65)
    %579 = arith.ori %570, %578 : i1 loc(#loc65)
    %580 = arith.andi %566, %579 : i1 loc(#loc65)
    %581 = arith.xori %569, %true_0 : i1 loc(#loc66)
    %582 = arith.xori %569, %true_0 : i1
    %583 = arith.ori %582, %575 : i1
    %584 = acir.extract %550 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc67)
    %c3_i32_26 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc68)
    %585 = arith.andi %584, %c3_i32_26 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc68)
    %c0_i32_27 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc69)
    %586 = arith.cmpi eq, %585, %c0_i32_27 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc69)
    %587 = arith.xori %586, %true_0 : i1 loc(#loc69)
    %588 = arith.andi %569, %587 : i1 loc(#loc69)
    %589 = arith.xori %588, %true_0 : i1
    %590 = arith.andi %575, %577 : i1
    %591 = arith.ori %589, %590 : i1
    %592 = arith.andi %588, %591 : i1
    %593 = arith.andi %569, %586 : i1
    %594 = arith.xori %593, %true_0 : i1
    %595 = arith.andi %575, %577 : i1
    %596 = arith.ori %594, %595 : i1
    %597 = arith.andi %596, %577 : i1
    %598 = arith.andi %593, %597 : i1
    %599 = arith.ori %592, %598 : i1
    %600 = arith.andi %583, %599 : i1
    %601 = arith.ori %581, %600 : i1
    %602 = arith.andi %580, %601 : i1
    %603 = arith.xori %569, %true_0 : i1 loc(#loc70)
    %604 = acir.current {aspect = "value", resource = "top.memory.words"} : !acir.array<i32, 16> loc(#loc70)
    %605 = arith.andi %583, %true : i1 loc(#loc70)
    %606 = arith.ori %603, %605 : i1 loc(#loc70)
    %607 = arith.andi %602, %606 : i1 loc(#loc70)
    %608 = arith.xori %569, %true_0 : i1 loc(#loc70)
    %609 = arith.andi %583, %true : i1 loc(#loc70)
    %610 = arith.ori %608, %609 : i1 loc(#loc70)
    %611 = arith.andi %607, %610 : i1 loc(#loc70)
    %612 = arith.xori %569, %true_0 : i1 loc(#loc70)
    %613 = arith.andi %574, %551 : i1 loc(#loc65)
    %614 = arith.andi %569, %613 : i1 loc(#loc65)
    %615 = arith.andi %true, %614 : i1 loc(#loc65)
    %616 = arith.andi %615, %577 : i1 loc(#loc65)
    %617 = arith.andi %583, %616 : i1 loc(#loc65)
    %618 = arith.ori %612, %617 : i1 loc(#loc65)
    %619 = arith.andi %611, %618 : i1 loc(#loc65)
    %620 = arith.xori %559, %true_0 : i1
    %621 = arith.xori %559, %true_0 : i1 loc(#loc71)
    %622 = arith.xori %559, %true_0 : i1
    %623 = arith.ori %622, %551 : i1
    %624 = arith.ori %621, %623 : i1
    %625 = arith.andi %623, %551 : i1 loc(#loc71)
    %626 = arith.andi %559, %625 : i1 loc(#loc71)
    %627 = arith.andi %624, %626 : i1 loc(#loc71)
    %628 = arith.ori %620, %627 : i1 loc(#loc71)
    %629 = arith.andi %619, %628 : i1 loc(#loc71)
    %630 = arith.xori %559, %true_0 : i1 loc(#loc72)
    %631 = arith.xori %559, %true_0 : i1
    %632 = arith.ori %631, %624 : i1
    %633 = acir.extract %550 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc73)
    %c3_i32_28 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc74)
    %634 = arith.andi %633, %c3_i32_28 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc74)
    %c0_i32_29 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc75)
    %635 = arith.cmpi eq, %634, %c0_i32_29 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc75)
    %636 = arith.xori %635, %true_0 : i1 loc(#loc75)
    %637 = arith.andi %559, %636 : i1 loc(#loc75)
    %638 = arith.xori %637, %true_0 : i1
    %639 = arith.andi %624, %626 : i1
    %640 = arith.ori %638, %639 : i1
    %641 = arith.andi %637, %640 : i1
    %642 = arith.andi %559, %635 : i1
    %643 = arith.xori %642, %true_0 : i1
    %644 = arith.andi %624, %626 : i1
    %645 = arith.ori %643, %644 : i1
    %646 = arith.andi %645, %626 : i1
    %647 = arith.andi %642, %646 : i1
    %648 = arith.ori %641, %647 : i1
    %649 = arith.andi %632, %648 : i1
    %650 = arith.ori %630, %649 : i1
    %651 = arith.andi %629, %650 : i1
    %652 = arith.xori %559, %true_0 : i1 loc(#loc76)
    %653 = arith.andi %632, %true : i1 loc(#loc76)
    %654 = arith.ori %652, %653 : i1 loc(#loc76)
    %655 = arith.andi %651, %654 : i1 loc(#loc76)
    %656 = arith.andi %551, %551 : i1 loc(#loc77)
    %657 = arith.andi %656, %551 : i1 loc(#loc77)
    %658 = arith.andi %563, %551 : i1
    %659 = arith.andi %560, %658 : i1
    %660 = arith.andi %true, %626 : i1 loc(#loc76)
    %661 = arith.andi %632, %660 : i1 loc(#loc76)
    %662 = arith.andi %559, %661 : i1 loc(#loc76)
    %663 = arith.andi %623, %662 : i1 loc(#loc76)
    %664 = arith.andi %559, %663 : i1 loc(#loc76)
    %665 = arith.ori %659, %664 : i1 loc(#loc76)
    %666 = arith.andi %657, %665 : i1 loc(#loc76)
    %667 = arith.andi %666, %551 : i1 loc(#loc76)
    %668 = arith.xori %667, %true_0 : i1 loc(#loc76)
    %669 = acir.current {aspect = "storage", resource = "top.retirement"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc78)
    %c0_i32_30 = arith.constant 0 : i32 loc(#loc78)
    %670 = acir.extract %669[%c0_i32_30] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc78)
    %c0_i32_31 = arith.constant 0 : i32 loc(#loc78)
    %671 = arith.cmpi ne, %29, %c0_i32_31 : i32 loc(#loc78)
    %672 = arith.andi %671, %671 : i1 loc(#loc78)
    %c0_i32_32 = arith.constant 0 : i32 loc(#loc78)
    %673 = arith.cmpi eq, %29, %c0_i32_32 : i32 loc(#loc78)
    %674 = arith.andi %672, %true : i1 loc(#loc78)
    %675 = arith.xori %673, %true_0 : i1 loc(#loc78)
    %676 = arith.andi %674, %675 : i1 loc(#loc78)
    %677 = arith.andi %676, %671 : i1 loc(#loc78)
    %678 = acir.extract %670 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc79)
    %679 = arith.xori %678, %true_0 : i1 loc(#loc79)
    %680 = arith.xori %679, %true_0 : i1
    %681 = arith.ori %680, %671 : i1
    %682 = arith.andi %679, %681 : i1
    %683 = arith.xori %678, %true_0 : i1
    %684 = arith.ori %683, %671 : i1
    %685 = arith.andi %684, %671 : i1
    %686 = arith.andi %678, %685 : i1
    %687 = arith.ori %682, %686 : i1
    %688 = arith.andi %677, %687 : i1
    %false_33 = arith.constant {acir.source_type = "bool"} false
    %689 = acir.extract %670 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc80)
    %c0_i32_34 = arith.constant {acir.source_type = "u32"} 0 : i32
    %690 = arith.cmpi ne, %689, %c0_i32_34 {acir.id = "v8", acir.source_type = "bool"} : i32
    %691 = arith.select %679, %false_33, %690 : i1
    %692 = arith.xori %691, %true_0 : i1
    %693 = arith.xori %691, %true_0 : i1
    %694 = arith.ori %693, %687 : i1
    %695 = arith.andi %694, %true : i1 loc(#loc81)
    %696 = arith.ori %692, %695 : i1 loc(#loc81)
    %697 = arith.andi %688, %696 : i1 loc(#loc81)
    %698 = arith.xori %691, %true_0 : i1 loc(#loc81)
    %699 = arith.andi %694, %true : i1 loc(#loc81)
    %700 = arith.ori %698, %699 : i1 loc(#loc81)
    %701 = arith.andi %697, %700 : i1 loc(#loc81)
    %702 = arith.xori %691, %true_0 : i1 loc(#loc81)
    %703 = arith.andi %true, %671 : i1 loc(#loc81)
    %704 = arith.andi %703, %671 : i1 loc(#loc81)
    %705 = arith.andi %694, %704 : i1 loc(#loc81)
    %706 = arith.ori %702, %705 : i1 loc(#loc81)
    %707 = arith.andi %701, %706 : i1 loc(#loc81)
    %708 = arith.xori %691, %true_0 : i1 loc(#loc82)
    %709 = arith.andi %694, %true : i1 loc(#loc82)
    %710 = arith.ori %708, %709 : i1 loc(#loc82)
    %711 = arith.andi %707, %710 : i1 loc(#loc82)
    %712 = arith.xori %691, %true_0 : i1 loc(#loc82)
    %713 = arith.andi %694, %true : i1 loc(#loc82)
    %714 = arith.ori %712, %713 : i1 loc(#loc82)
    %715 = arith.andi %711, %714 : i1 loc(#loc82)
    %716 = arith.xori %691, %true_0 : i1 loc(#loc82)
    %717 = arith.andi %true, %671 : i1 loc(#loc82)
    %718 = arith.andi %694, %717 : i1 loc(#loc82)
    %719 = arith.ori %716, %718 : i1 loc(#loc82)
    %720 = arith.andi %715, %719 : i1 loc(#loc82)
    %721 = acir.current {aspect = "value", resource = "top.writeback.retired"} : i32 loc(#loc83)
    %722 = arith.andi %720, %true : i1 loc(#loc83)
    %723 = arith.andi %722, %true : i1 loc(#loc83)
    %724 = arith.andi %723, %671 : i1 loc(#loc83)
    %725 = acir.fire "top.writeback.commit#4" %724 {successors = [3], txn = 4 : i64} loc(#loc83)
    %c0_i32_35 = arith.constant 0 : i32 loc(#loc83)
    %c1_i32_36 = arith.constant 1 : i32 loc(#loc83)
    %c0_i32_37 = arith.constant 0 : i32 loc(#loc83)
    %726 = arith.select %725, %c1_i32_36, %c0_i32_37 : i32 loc(#loc83)
    %727 = arith.addi %c0_i32_35, %726 : i32 loc(#loc83)
    %728 = arith.subi %29, %727 : i32 loc(#loc84)
    %c1_i32_38 = arith.constant 1 : i32 loc(#loc83)
    %729 = arith.cmpi uge, %728, %c1_i32_38 : i32 loc(#loc83)
    %730 = arith.ori %668, %true : i1 loc(#loc83)
    %731 = arith.andi %655, %730 : i1 loc(#loc83)
    %732 = arith.xori %667, %true_0 : i1 loc(#loc84)
    %733 = arith.xori %729, %true_0 : i1 loc(#loc84)
    %734 = arith.ori %732, %733 : i1 loc(#loc84)
    %735 = arith.andi %731, %734 : i1 loc(#loc84)
    %736 = acir.fire "top.memory_stage.access#3" %735 {successors = [2], txn = 3 : i64} loc(#loc84)
    %c0_i32_39 = arith.constant 0 : i32 loc(#loc84)
    %c1_i32_40 = arith.constant 1 : i32 loc(#loc84)
    %c0_i32_41 = arith.constant 0 : i32 loc(#loc84)
    %737 = arith.select %736, %c1_i32_40, %c0_i32_41 : i32 loc(#loc84)
    %738 = arith.addi %c0_i32_39, %737 : i32 loc(#loc84)
    %739 = arith.subi %548, %738 : i32 loc(#loc60)
    %c1_i32_42 = arith.constant 1 : i32 loc(#loc84)
    %740 = arith.cmpi uge, %739, %c1_i32_42 : i32 loc(#loc84)
    %741 = arith.ori %547, %true : i1 loc(#loc84)
    %742 = arith.andi %526, %741 : i1 loc(#loc84)
    %743 = arith.xori %546, %true_0 : i1 loc(#loc60)
    %744 = arith.xori %740, %true_0 : i1 loc(#loc60)
    %745 = arith.ori %743, %744 : i1 loc(#loc60)
    %746 = arith.andi %742, %745 : i1 loc(#loc60)
    %747 = acir.fire "top.execute.execute#2" %746 {successors = [1], txn = 2 : i64} loc(#loc60)
    %c0_i32_43 = arith.constant 0 : i32 loc(#loc60)
    %c1_i32_44 = arith.constant 1 : i32 loc(#loc60)
    %c0_i32_45 = arith.constant 0 : i32 loc(#loc60)
    %748 = arith.select %747, %c1_i32_44, %c0_i32_45 : i32 loc(#loc60)
    %749 = arith.addi %c0_i32_43, %748 : i32 loc(#loc60)
    %750 = arith.subi %517, %749 : i32 loc(#loc53)
    %c1_i32_46 = arith.constant 1 : i32 loc(#loc60)
    %751 = arith.cmpi uge, %750, %c1_i32_46 : i32 loc(#loc60)
    %752 = arith.andi %346, %true : i1 loc(#loc60)
    %753 = arith.ori %516, %752 : i1 loc(#loc60)
    %754 = arith.andi %515, %753 : i1 loc(#loc60)
    %755 = arith.xori %512, %true_0 : i1 loc(#loc53)
    %756 = arith.xori %751, %true_0 : i1 loc(#loc53)
    %757 = arith.andi %346, %756 : i1 loc(#loc53)
    %758 = arith.ori %755, %757 : i1 loc(#loc53)
    %759 = arith.andi %754, %758 : i1 loc(#loc53)
    %760 = acir.fire "top.decode_stage.read_operands#1" %759 {successors = [0], txn = 1 : i64} loc(#loc53)
    %761 = arith.andi %760, %107 : i1 loc(#loc53)
    %c0_i32_47 = arith.constant 0 : i32 loc(#loc53)
    %c1_i32_48 = arith.constant 1 : i32 loc(#loc53)
    %c0_i32_49 = arith.constant 0 : i32 loc(#loc53)
    %762 = arith.select %761, %c1_i32_48, %c0_i32_49 : i32 loc(#loc53)
    %763 = arith.addi %c0_i32_47, %762 : i32 loc(#loc53)
    %764 = arith.subi %28, %763 : i32 loc(#loc6)
    %c1_i32_50 = arith.constant 1 : i32 loc(#loc53)
    %765 = arith.cmpi uge, %764, %c1_i32_50 : i32 loc(#loc53)
    %766 = arith.andi %3, %true : i1 loc(#loc53)
    %767 = arith.ori %27, %766 : i1 loc(#loc53)
    %768 = arith.andi %26, %767 : i1 loc(#loc53)
    %769 = arith.xori %23, %true_0 : i1 loc(#loc6)
    %770 = arith.xori %765, %true_0 : i1 loc(#loc6)
    %771 = arith.andi %3, %770 : i1 loc(#loc6)
    %772 = arith.ori %769, %771 : i1 loc(#loc6)
    %773 = arith.andi %768, %772 : i1 loc(#loc6)
    %774 = acir.fire "top.fetch.fetch#0" %773 {successors = [], txn = 0 : i64} loc(#loc6)
    %c0_i32_51 = arith.constant 0 : i32 loc(#loc6)
    %c1_i32_52 = arith.constant 1 : i32 loc(#loc6)
    %c0_i32_53 = arith.constant 0 : i32 loc(#loc6)
    %775 = arith.select %747, %c1_i32_52, %c0_i32_53 : i32 loc(#loc6)
    %776 = arith.addi %c0_i32_51, %775 : i32 loc(#loc6)
    %777 = arith.subi %517, %776 : i32 loc(#loc6)
    %778 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc6)
    %c0_i32_54 = arith.constant 0 : i32 loc(#loc6)
    %779 = acir.extract %518[%c0_i32_54] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc6)
    %c0_i32_55 = arith.constant 0 : i32 loc(#loc6)
    %780 = arith.cmpi eq, %776, %c0_i32_55 : i32 loc(#loc6)
    %781 = arith.select %780, %779, %778 : !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc6)
    %782 = acir.aggregate(%781) : (!acir.struct<"src_isa_f0424bc8.Operands">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc6)
    %783 = arith.andi %760, %512 : i1 loc(#loc53)
    %784 = acir.extract %85 {acir.id = "v17", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc85)
    %785 = acir.extract %85 {acir.id = "v18", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc86)
    %786 = acir.extract %85 {acir.id = "v19", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc87)
    %787 = acir.extract %85 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc88)
    %788 = acir.extract %389[%381] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc43)
    %789 = arith.select %384, %788, %c0_i32_14 : i32 loc(#loc43)
    %790 = acir.extract %389[%401] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc43)
    %791 = arith.select %404, %790, %c0_i32_15 : i32 loc(#loc43)
    %792 = acir.extract %85 {acir.id = "v12", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc89)
    %793 = arith.select %483, %791, %792 : i32 loc(#loc89)
    %794 = acir.extract %85 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc90)
    %795 = acir.aggregate(%784, %785, %786, %787, %789, %793, %791, %794) {acir.id = "v22", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc90)
    %c0_i32_56 = arith.constant 0 : i32 loc(#loc90)
    %796 = arith.select %783, %777, %c0_i32_56 : i32 loc(#loc90)
    %797 = acir.update %782, %795[%796] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1>, !acir.struct<"src_isa_f0424bc8.Operands">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc90)
    %798 = arith.select %783, %797, %782 : !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc90)
    %c1_i32_57 = arith.constant 1 : i32 loc(#loc90)
    %c0_i32_58 = arith.constant 0 : i32 loc(#loc90)
    %799 = arith.select %783, %c1_i32_57, %c0_i32_58 : i32 loc(#loc90)
    %800 = arith.addi %777, %799 : i32 loc(#loc90)
    acir.next %798 {aspect = "storage", resource = "top.decode_stage.out"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc90)
    acir.next %800 {aspect = "count", resource = "top.decode_stage.out"} : i32 loc(#loc90)
    %c0_i32_59 = arith.constant 0 : i32 loc(#loc90)
    %c1_i32_60 = arith.constant 1 : i32 loc(#loc90)
    %c0_i32_61 = arith.constant 0 : i32 loc(#loc90)
    %801 = arith.select %736, %c1_i32_60, %c0_i32_61 : i32 loc(#loc90)
    %802 = arith.addi %c0_i32_59, %801 : i32 loc(#loc90)
    %803 = arith.subi %548, %802 : i32 loc(#loc90)
    %804 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc90)
    %c0_i32_62 = arith.constant 0 : i32 loc(#loc90)
    %805 = acir.extract %549[%c0_i32_62] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc90)
    %c0_i32_63 = arith.constant 0 : i32 loc(#loc90)
    %806 = arith.cmpi eq, %802, %c0_i32_63 : i32 loc(#loc90)
    %807 = arith.select %806, %805, %804 : !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc90)
    %808 = acir.aggregate(%807) : (!acir.struct<"src_isa_f0424bc8.Executed">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc90)
    %809 = arith.andi %747, %546 : i1 loc(#loc60)
    %810 = acir.extract %519 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc91)
    %811 = acir.extract %519 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc92)
    %812 = acir.extract %519 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc93)
    %813 = acir.extract %519 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc94)
    %814 = acir.extract %519 {acir.id = "v12", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc58)
    %815 = acir.extract %519 {acir.id = "v13", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc95)
    %816 = arith.addi %814, %815 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc95)
    %817 = acir.extract %519 {acir.id = "v9", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc59)
    %818 = acir.extract %519 {acir.id = "v10", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc96)
    %819 = arith.subi %817, %818 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc96)
    %820 = arith.select %532, %816, %819 : i32 loc(#loc96)
    %821 = acir.extract %519 {acir.id = "v22", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc97)
    %822 = acir.extract %519 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc98)
    %823 = acir.aggregate(%810, %811, %812, %813, %820, %821, %822) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc98)
    %c0_i32_64 = arith.constant 0 : i32 loc(#loc98)
    %824 = arith.select %809, %803, %c0_i32_64 : i32 loc(#loc98)
    %825 = acir.update %808, %823[%824] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1>, !acir.struct<"src_isa_f0424bc8.Executed">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc98)
    %826 = arith.select %809, %825, %808 : !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc98)
    %c1_i32_65 = arith.constant 1 : i32 loc(#loc98)
    %c0_i32_66 = arith.constant 0 : i32 loc(#loc98)
    %827 = arith.select %809, %c1_i32_65, %c0_i32_66 : i32 loc(#loc98)
    %828 = arith.addi %803, %827 : i32 loc(#loc98)
    acir.next %826 {aspect = "storage", resource = "top.execute.out"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc98)
    acir.next %828 {aspect = "count", resource = "top.execute.out"} : i32 loc(#loc98)
    %c0_i32_67 = arith.constant 0 : i32 loc(#loc98)
    %c1_i32_68 = arith.constant 1 : i32 loc(#loc98)
    %c0_i32_69 = arith.constant 0 : i32 loc(#loc98)
    %829 = arith.select %761, %c1_i32_68, %c0_i32_69 : i32 loc(#loc98)
    %830 = arith.addi %c0_i32_67, %829 : i32 loc(#loc98)
    %831 = arith.subi %28, %830 : i32 loc(#loc98)
    %832 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc98)
    %c0_i32_70 = arith.constant 0 : i32 loc(#loc98)
    %833 = acir.extract %33[%c0_i32_70] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc98)
    %c0_i32_71 = arith.constant 0 : i32 loc(#loc98)
    %834 = arith.cmpi eq, %830, %c0_i32_71 : i32 loc(#loc98)
    %835 = arith.select %834, %833, %832 : !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc98)
    %836 = acir.aggregate(%835) : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc98)
    %837 = arith.andi %774, %23 : i1 loc(#loc6)
    %c2_i32_72 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc99)
    %838 = arith.shrui %0, %c2_i32_72 : i32 loc(#loc99)
    %839 = acir.extract %9[%838] {acir.id = "v8", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32 loc(#loc99)
    %840 = acir.aggregate(%0, %839) {acir.id = "v13", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc99)
    %c0_i32_73 = arith.constant 0 : i32 loc(#loc99)
    %841 = arith.select %837, %831, %c0_i32_73 : i32 loc(#loc99)
    %842 = acir.update %836, %840[%841] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1>, !acir.struct<"src_isa_f0424bc8.Fetched">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc99)
    %843 = arith.select %837, %842, %836 : !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc99)
    %c1_i32_74 = arith.constant 1 : i32 loc(#loc99)
    %c0_i32_75 = arith.constant 0 : i32 loc(#loc99)
    %844 = arith.select %837, %c1_i32_74, %c0_i32_75 : i32 loc(#loc99)
    %845 = arith.addi %831, %844 : i32 loc(#loc99)
    acir.next %843 {aspect = "storage", resource = "top.fetch.out"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc99)
    acir.next %845 {aspect = "count", resource = "top.fetch.out"} : i32 loc(#loc99)
    %846 = arith.andi %774, %1 : i1 loc(#loc6)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc100)
    %847 = arith.addi %0, %c4_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc100)
    %848 = arith.select %846, %847, %0 : i32 loc(#loc100)
    acir.next %848 {aspect = "value", resource = "top.fetch.pc"} : i32 loc(#loc100)
    acir.next %9 {aspect = "value", resource = "top.fetch.rom"} : !acir.array<i32, 6> loc(#loc100)
    %849 = arith.andi %736, %569 : i1 loc(#loc84)
    %850 = acir.extract %550 {acir.id = "v17", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc101)
    %c2_i32_76 = arith.constant {acir.source_type = "u32"} 2 : i32
    %851 = arith.shrui %584, %c2_i32_76 : i32
    %852 = acir.update %604, %850[%851] {acir.id = "v28", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16>
    %853 = arith.select %849, %852, %604 : !acir.array<i32, 16>
    acir.next %853 {aspect = "value", resource = "top.memory.words"} : !acir.array<i32, 16>
    %c0_i32_77 = arith.constant 0 : i32
    %c1_i32_78 = arith.constant 1 : i32
    %c0_i32_79 = arith.constant 0 : i32
    %854 = arith.select %725, %c1_i32_78, %c0_i32_79 : i32
    %855 = arith.addi %c0_i32_77, %854 : i32
    %856 = arith.subi %29, %855 : i32
    %857 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Retired">
    %c0_i32_80 = arith.constant 0 : i32
    %858 = acir.extract %669[%c0_i32_80] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Retired">
    %c0_i32_81 = arith.constant 0 : i32
    %859 = arith.cmpi eq, %855, %c0_i32_81 : i32
    %860 = arith.select %859, %858, %857 : !acir.struct<"src_isa_f0424bc8.Retired">
    %861 = acir.aggregate(%860) : (!acir.struct<"src_isa_f0424bc8.Retired">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1>
    %862 = arith.andi %736, %667 : i1 loc(#loc84)
    %863 = acir.extract %550 {acir.id = "v29", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc102)
    %864 = acir.extract %550 {acir.id = "v30", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc103)
    %865 = acir.extract %550 {acir.id = "v31", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc104)
    %866 = acir.extract %550 {acir.id = "v5", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc105)
    %c2_i32_82 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc106)
    %867 = arith.shrui %633, %c2_i32_82 : i32 loc(#loc106)
    %868 = acir.extract %604[%867] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc106)
    %869 = arith.select %560, %866, %868 : i32 loc(#loc106)
    %870 = acir.extract %550 {acir.id = "v32", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc107)
    %871 = acir.aggregate(%863, %864, %865, %869, %870) {acir.id = "v33", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc107)
    %c0_i32_83 = arith.constant 0 : i32 loc(#loc107)
    %872 = arith.select %862, %856, %c0_i32_83 : i32 loc(#loc107)
    %873 = acir.update %861, %871[%872] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1>, !acir.struct<"src_isa_f0424bc8.Retired">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc107)
    %874 = arith.select %862, %873, %861 : !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc107)
    %c1_i32_84 = arith.constant 1 : i32 loc(#loc107)
    %c0_i32_85 = arith.constant 0 : i32 loc(#loc107)
    %875 = arith.select %862, %c1_i32_84, %c0_i32_85 : i32 loc(#loc107)
    %876 = arith.addi %856, %875 : i32 loc(#loc107)
    acir.next %874 {aspect = "storage", resource = "top.retirement"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc107)
    acir.next %876 {aspect = "count", resource = "top.retirement"} : i32 loc(#loc107)
    %877 = arith.andi %725, %691 : i1
    %878 = acir.extract %670 {acir.id = "v36", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc108)
    %879 = acir.update %32, %false_33[%878] {acir.id = "v37", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc108)
    %880 = arith.select %877, %879, %32 : !acir.array<i1, 32> loc(#loc108)
    %881 = arith.andi %760, %449 : i1 loc(#loc53)
    %true_86 = arith.constant {acir.source_type = "bool"} true
    %882 = acir.extract %85 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc109)
    %883 = acir.update %32, %true_86[%882] {acir.id = "v24", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc109)
    %884 = arith.select %881, %883, %880 : !acir.array<i1, 32> loc(#loc109)
    acir.next %884 {aspect = "value", resource = "top.rf.pending"} : !acir.array<i1, 32> loc(#loc109)
    %885 = arith.andi %725, %691 : i1
    %886 = acir.extract %670 {acir.id = "v15", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc110)
    %887 = acir.extract %670 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc111)
    %888 = acir.update %389, %886[%887] {acir.id = "v24", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc111)
    %889 = arith.select %885, %888, %389 : !acir.array<i32, 32> loc(#loc111)
    acir.next %889 {aspect = "value", resource = "top.rf.registers"} : !acir.array<i32, 32> loc(#loc111)
    %890 = acir.current {aspect = "value", resource = "top.writeback.last_pc"} : i32 loc(#loc111)
    %891 = acir.extract %670 {acir.id = "v12", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc112)
    %892 = arith.select %725, %891, %890 : i32 loc(#loc112)
    acir.next %892 {aspect = "value", resource = "top.writeback.last_pc"} : i32 loc(#loc112)
    %c1_i32_87 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc113)
    %893 = arith.addi %721, %c1_i32_87 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc113)
    %894 = arith.select %725, %893, %721 : i32 loc(#loc113)
    acir.next %894 {aspect = "value", resource = "top.writeback.retired"} : i32 loc(#loc113)
    %895 = arith.andi %736, %569 : i1 loc(#loc84)
    %false_88 = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %896 = arith.cmpi ult, %584, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32
    %897 = arith.select %588, %false_88, %896 : i1
    acir.effect %895(%897) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc66)
    %898 = arith.andi %736, %559 : i1 loc(#loc84)
    %false_89 = arith.constant {acir.source_type = "bool"} false
    %c64_i32_90 = arith.constant {acir.source_type = "u32"} 64 : i32
    %899 = arith.cmpi ult, %633, %c64_i32_90 {acir.id = "v10", acir.source_type = "bool"} : i32
    %900 = arith.select %637, %false_89, %899 : i1
    acir.effect %898(%900) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc72)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":74:22)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":75:25)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":76:19)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":70:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":71:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":96:19)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:20)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":95:23)
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
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":96:42)
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
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:20)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":98:19)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:15)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:23)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:16)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":89:23)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":89:31)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":91:9)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:12)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:28)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:13)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:16)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:9)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:36)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:9)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":109:16)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:16)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":110:41)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":110:54)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":110:63)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":110:18)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:9)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:16)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":127:12)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":127:25)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":129:14)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":130:13)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":53:9)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":130:26)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":53:26)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":53:31)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:9)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":128:22)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":48:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":128:34)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":48:26)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":48:31)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:16)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":131:16)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":146:16)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:12)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:28)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:13)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":148:19)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":131:9)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:25)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:37)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:51)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:65)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:15)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:100)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:25)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:34)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:45)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:56)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":110:74)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":110:29)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:73)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:91)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:34)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":70:27)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":130:38)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":131:24)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":131:33)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":131:44)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":126:18)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:33)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":131:61)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:21)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:21)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:34)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:23)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":149:19)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":148:29)

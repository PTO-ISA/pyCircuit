module attributes {acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, exports = {}, format_version = 8 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", type = "bool"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default, name = "lhs", type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", type = "bool"}, {default = false, name = "done", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}]}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.adder.out", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.adder.out", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cdb.event", source_branch = 29 : i64, target_branches = [59], to = 8 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.multiplier.out", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.multiplier.out", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}], schedule = [3, 1, 5, 2, 0, 4, 6, 7, 8], stage = "rtl"} {
    acir.state @top.adder.out : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.audit.last_tag : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.commit.last_tag : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.commit.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]], kind = "cell"}
    acir.state @top.multiplier.active : i1 {initial = false, kind = "cell"}
    acir.state @top.multiplier.out : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.multiplier.remaining : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]], kind = "cell"}
    acir.state @top.rf.committed : !acir.array<i32, 8> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rf.registers : !acir.array<i32, 8> {initial = [0 : i32, 0 : i32, 6 : i32, 7 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rf.renamed : !acir.array<i32, 8> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.right.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]], kind = "cell"}
    acir.state @top.rob.head : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.rob.slot0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slot1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slot2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slot3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.tail : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.scheduler.add_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.scheduler.mul_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity = 4 : i64, initial = [[1 : i32, 1 : i32, 2 : i32, 3 : i32], [0 : i32, 4 : i32, 1 : i32, 2 : i32], [0 : i32, 5 : i32, 2 : i32, 3 : i32], [0 : i32, 6 : i32, 4 : i32, 5 : i32]], kind = "queue"}
    %true = arith.constant true
    %false = arith.constant false
    %0 = acir.current {aspect = "value", resource = "top.left.slot"} : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc1)
    %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc1)
    %true_0 = arith.constant true
    %2 = arith.xori %1, %true_0 : i1
    %false_1 = arith.constant {acir.source_type = "bool"} false
    %3 = acir.extract %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc2)
    %4 = acir.extract %3 {acir.id = "v8", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc2)
    %5 = arith.select %2, %false_1, %4 : i1 loc(#loc2)
    %6 = arith.xori %5, %true_0 : i1 loc(#loc2)
    %7 = acir.extract %0 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc3)
    %8 = acir.extract %7 {acir.id = "v15", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc3)
    %9 = arith.select %6, %false_1, %8 : i1 loc(#loc3)
    %10 = arith.xori %9, %true_0 : i1 loc(#loc3)
    %11 = acir.extract %0 {acir.id = "v36", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc4)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %12 = arith.cmpi eq, %11, %c0_i32 {acir.id = "v38", acir.source_type = "bool"} : i32
    %13 = arith.select %10, %false_1, %12 : i1
    %14 = arith.xori %13, %true_0 : i1
    %15 = arith.xori %14, %true_0 : i1
    %16 = arith.xori %10, %true_0 : i1
    %17 = arith.xori %6, %true_0 : i1
    %18 = arith.xori %2, %true_0 : i1
    %19 = arith.ori %18, %true : i1
    %20 = arith.andi %2, %19 : i1
    %21 = arith.xori %1, %true_0 : i1
    %22 = arith.ori %21, %true : i1
    %23 = arith.andi %22, %true : i1
    %24 = arith.andi %1, %23 : i1
    %25 = arith.ori %20, %24 : i1
    %26 = arith.ori %17, %25 : i1
    %27 = arith.andi %6, %26 : i1
    %28 = arith.xori %5, %true_0 : i1
    %29 = arith.ori %28, %25 : i1
    %30 = arith.andi %29, %true : i1
    %31 = arith.andi %5, %30 : i1
    %32 = arith.ori %27, %31 : i1
    %33 = arith.ori %16, %32 : i1
    %34 = arith.andi %10, %33 : i1
    %35 = arith.xori %9, %true_0 : i1
    %36 = arith.ori %35, %32 : i1
    %37 = arith.andi %36, %true : i1
    %38 = arith.andi %9, %37 : i1
    %39 = arith.ori %34, %38 : i1
    %40 = arith.ori %15, %39 : i1
    %41 = arith.andi %14, %40 : i1
    %42 = arith.xori %13, %true_0 : i1
    %43 = arith.xori %13, %true_0 : i1
    %44 = arith.ori %43, %39 : i1
    %45 = arith.ori %42, %44 : i1
    %46 = acir.current {aspect = "value", resource = "top.right.slot"} : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc5)
    %47 = acir.extract %46 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc5)
    %48 = arith.xori %47, %true_0 : i1 loc(#loc5)
    %49 = acir.extract %46 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc6)
    %50 = acir.extract %49 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc6)
    %51 = arith.select %48, %false_1, %50 : i1 loc(#loc6)
    %52 = arith.xori %51, %true_0 : i1 loc(#loc6)
    %53 = acir.extract %46 {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc7)
    %54 = acir.extract %53 {acir.id = "v30", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc7)
    %55 = arith.select %52, %false_1, %54 : i1 loc(#loc7)
    %56 = arith.xori %55, %true_0 : i1 loc(#loc7)
    %57 = acir.extract %46 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc8)
    %58 = arith.cmpi eq, %57, %c0_i32 {acir.id = "v46", acir.source_type = "bool"} : i32 loc(#loc8)
    %59 = arith.select %56, %false_1, %58 : i1 loc(#loc8)
    %60 = acir.unary %59 "not" {acir.id = "v68", acir.source_type = "bool"} : i1 -> i1 loc(#loc8)
    %61 = arith.xori %60, %true_0 : i1 loc(#loc8)
    %62 = arith.andi %13, %61 : i1 loc(#loc8)
    %63 = arith.xori %62, %true_0 : i1
    %64 = arith.xori %56, %true_0 : i1
    %65 = arith.xori %52, %true_0 : i1
    %66 = arith.xori %48, %true_0 : i1
    %67 = arith.ori %66, %true : i1
    %68 = arith.andi %48, %67 : i1
    %69 = arith.xori %47, %true_0 : i1
    %70 = arith.ori %69, %true : i1
    %71 = arith.andi %70, %true : i1
    %72 = arith.andi %47, %71 : i1
    %73 = arith.ori %68, %72 : i1
    %74 = arith.ori %65, %73 : i1
    %75 = arith.andi %52, %74 : i1
    %76 = arith.xori %51, %true_0 : i1
    %77 = arith.ori %76, %73 : i1
    %78 = arith.andi %77, %true : i1
    %79 = arith.andi %51, %78 : i1
    %80 = arith.ori %75, %79 : i1
    %81 = arith.ori %64, %80 : i1
    %82 = arith.andi %56, %81 : i1
    %83 = arith.xori %55, %true_0 : i1
    %84 = arith.ori %83, %80 : i1
    %85 = arith.andi %84, %true : i1
    %86 = arith.andi %55, %85 : i1
    %87 = arith.ori %82, %86 : i1
    %88 = arith.andi %44, %87 : i1
    %89 = arith.ori %63, %88 : i1
    %90 = arith.andi %true, %true : i1 loc(#loc9)
    %91 = arith.andi %89, %90 : i1 loc(#loc9)
    %92 = arith.andi %62, %91 : i1 loc(#loc9)
    %93 = arith.andi %13, %60 : i1
    %94 = arith.xori %93, %true_0 : i1
    %95 = arith.andi %44, %87 : i1
    %96 = arith.ori %94, %95 : i1
    %97 = arith.andi %93, %96 : i1
    %98 = arith.ori %92, %97 : i1
    %99 = arith.andi %45, %98 : i1
    %100 = arith.andi %13, %99 : i1
    %101 = arith.ori %41, %100 : i1
    %102 = arith.andi %101, %101 : i1
    %103 = acir.extract %0 {acir.id = "v71", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc9)
    %104 = acir.extract %46 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc10)
    %105 = arith.cmpi ult, %103, %104 {acir.id = "v73", acir.source_type = "bool"} : i32 loc(#loc10)
    %true_2 = arith.constant {acir.source_type = "bool"} true
    %106 = arith.select %62, %105, %true_2 : i1
    %107 = arith.select %14, %false_1, %106 : i1
    %108 = arith.xori %107, %true_0 : i1
    %109 = arith.xori %108, %true_0 : i1
    %110 = arith.xori %108, %true_0 : i1 loc(#loc11)
    %111 = arith.xori %108, %true_0 : i1 loc(#loc11)
    %112 = arith.xori %108, %true_0 : i1
    %113 = arith.ori %112, %101 : i1
    %114 = arith.ori %111, %113 : i1
    %115 = arith.ori %110, %114 : i1
    %116 = arith.andi %115, %true : i1 loc(#loc12)
    %117 = arith.ori %109, %116 : i1 loc(#loc12)
    %118 = arith.andi %102, %117 : i1 loc(#loc12)
    %119 = arith.xori %108, %true_0 : i1
    %120 = arith.andi %115, %true : i1
    %121 = arith.ori %119, %120 : i1
    %122 = arith.andi %118, %121 : i1
    %123 = arith.xori %108, %true_0 : i1
    %124 = arith.xori %108, %true_0 : i1
    %125 = arith.ori %124, %115 : i1
    %126 = acir.extract %46 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc13)
    %127 = arith.xori %126, %true_0 : i1 loc(#loc13)
    %128 = arith.andi %108, %127 : i1 loc(#loc13)
    %129 = arith.xori %128, %true_0 : i1
    %130 = arith.andi %115, %true : i1
    %131 = arith.ori %129, %130 : i1
    %132 = arith.andi %128, %131 : i1
    %133 = arith.andi %108, %126 : i1
    %134 = arith.xori %133, %true_0 : i1
    %135 = arith.andi %115, %true : i1
    %136 = arith.ori %134, %135 : i1
    %137 = arith.andi %108, %113 : i1 loc(#loc11)
    %138 = arith.andi %114, %137 : i1 loc(#loc11)
    %139 = arith.andi %108, %138 : i1 loc(#loc11)
    %140 = arith.andi %true, %139 : i1 loc(#loc11)
    %141 = arith.andi %136, %140 : i1 loc(#loc11)
    %142 = arith.andi %133, %141 : i1 loc(#loc11)
    %143 = arith.ori %132, %142 : i1 loc(#loc11)
    %144 = arith.andi %125, %143 : i1 loc(#loc11)
    %145 = arith.ori %123, %144 : i1 loc(#loc11)
    %146 = arith.andi %122, %145 : i1 loc(#loc11)
    %147 = arith.xori %108, %true_0 : i1
    %148 = arith.xori %108, %true_0 : i1
    %149 = arith.ori %148, %125 : i1
    %false_3 = arith.constant {acir.source_type = "bool"} false
    %150 = acir.extract %46 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc14)
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32
    %151 = arith.cmpi eq, %150, %c0_i32_4 {acir.id = "v7", acir.source_type = "bool"} : i32
    %152 = arith.select %128, %false_3, %151 : i1
    %153 = arith.xori %152, %true_0 : i1
    %154 = arith.andi %108, %153 : i1
    %155 = arith.xori %154, %true_0 : i1
    %156 = arith.andi %125, %143 : i1
    %157 = arith.ori %155, %156 : i1
    %158 = arith.andi %154, %157 : i1
    %159 = arith.andi %108, %152 : i1
    %160 = arith.xori %159, %true_0 : i1
    %161 = arith.andi %125, %143 : i1
    %162 = arith.ori %160, %161 : i1
    %163 = arith.andi %162, %true : i1
    %164 = arith.andi %159, %163 : i1
    %165 = arith.ori %158, %164 : i1
    %166 = arith.andi %149, %165 : i1
    %167 = arith.ori %147, %166 : i1
    %168 = arith.andi %146, %167 : i1
    %169 = arith.xori %108, %true_0 : i1 loc(#loc15)
    %170 = arith.xori %108, %true_0 : i1
    %171 = arith.ori %170, %149 : i1
    %172 = acir.extract %46 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %173 = acir.extract %172 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc16)
    %174 = arith.select %154, %false_3, %173 : i1 loc(#loc16)
    %175 = arith.xori %174, %true_0 : i1 loc(#loc16)
    %176 = arith.andi %108, %175 : i1 loc(#loc16)
    %177 = arith.xori %176, %true_0 : i1
    %178 = arith.andi %149, %165 : i1
    %179 = arith.ori %177, %178 : i1
    %180 = arith.andi %176, %179 : i1
    %181 = arith.andi %108, %174 : i1
    %182 = arith.xori %181, %true_0 : i1
    %183 = arith.andi %149, %165 : i1
    %184 = arith.ori %182, %183 : i1
    %185 = arith.andi %184, %true : i1
    %186 = arith.andi %181, %185 : i1
    %187 = arith.ori %180, %186 : i1
    %188 = arith.andi %171, %187 : i1
    %189 = arith.andi %108, %188 : i1
    %190 = arith.andi %114, %189 : i1
    %191 = arith.andi %108, %190 : i1
    %192 = acir.extract %46 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc17)
    %193 = acir.extract %192 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc17)
    %194 = arith.select %176, %false_3, %193 : i1 loc(#loc17)
    %195 = arith.andi %191, %194 : i1 loc(#loc17)
    %196 = arith.andi %113, %195 : i1 loc(#loc17)
    %197 = arith.ori %169, %196 : i1 loc(#loc17)
    %198 = arith.andi %168, %197 : i1 loc(#loc17)
    %199 = arith.xori %108, %true_0 : i1 loc(#loc18)
    %200 = arith.xori %108, %true_0 : i1 loc(#loc15)
    %201 = arith.andi %113, %195 : i1 loc(#loc15)
    %202 = arith.ori %200, %201 : i1 loc(#loc15)
    %203 = arith.andi %202, %true : i1 loc(#loc18)
    %204 = arith.ori %199, %203 : i1 loc(#loc18)
    %205 = arith.andi %198, %204 : i1 loc(#loc18)
    %206 = arith.xori %108, %true_0 : i1 loc(#loc19)
    %207 = arith.ori %206, %202 : i1 loc(#loc19)
    %208 = arith.andi %205, %207 : i1 loc(#loc19)
    %209 = arith.xori %107, %true_0 : i1 loc(#loc12)
    %210 = arith.xori %107, %true_0 : i1 loc(#loc20)
    %211 = arith.xori %107, %true_0 : i1 loc(#loc20)
    %212 = arith.xori %107, %true_0 : i1 loc(#loc21)
    %213 = arith.ori %212, %101 : i1 loc(#loc21)
    %214 = arith.ori %211, %213 : i1 loc(#loc21)
    %215 = arith.ori %210, %214 : i1 loc(#loc21)
    %216 = arith.andi %215, %true : i1 loc(#loc12)
    %217 = arith.ori %209, %216 : i1 loc(#loc12)
    %218 = arith.andi %208, %217 : i1 loc(#loc12)
    %219 = arith.xori %107, %true_0 : i1
    %220 = arith.andi %215, %true : i1
    %221 = arith.ori %219, %220 : i1
    %222 = arith.andi %218, %221 : i1
    %223 = arith.xori %107, %true_0 : i1
    %224 = arith.xori %107, %true_0 : i1
    %225 = arith.ori %224, %215 : i1
    %226 = acir.extract %0 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc13)
    %227 = arith.xori %226, %true_0 : i1 loc(#loc13)
    %228 = arith.andi %107, %227 : i1 loc(#loc13)
    %229 = arith.xori %228, %true_0 : i1
    %230 = arith.andi %215, %true : i1
    %231 = arith.ori %229, %230 : i1
    %232 = arith.andi %228, %231 : i1
    %233 = arith.andi %107, %226 : i1
    %234 = arith.xori %233, %true_0 : i1
    %235 = arith.andi %215, %true : i1
    %236 = arith.ori %234, %235 : i1
    %237 = arith.andi %107, %213 : i1 loc(#loc20)
    %238 = arith.andi %214, %237 : i1 loc(#loc20)
    %239 = arith.andi %107, %238 : i1 loc(#loc20)
    %240 = arith.andi %true, %239 : i1 loc(#loc20)
    %241 = arith.andi %236, %240 : i1 loc(#loc20)
    %242 = arith.andi %233, %241 : i1 loc(#loc20)
    %243 = arith.ori %232, %242 : i1 loc(#loc20)
    %244 = arith.andi %225, %243 : i1 loc(#loc20)
    %245 = arith.ori %223, %244 : i1 loc(#loc20)
    %246 = arith.andi %222, %245 : i1 loc(#loc20)
    %247 = arith.xori %107, %true_0 : i1
    %248 = arith.xori %107, %true_0 : i1
    %249 = arith.ori %248, %225 : i1
    %false_5 = arith.constant {acir.source_type = "bool"} false
    %250 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc14)
    %251 = arith.cmpi eq, %250, %c0_i32_4 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc14)
    %252 = arith.select %228, %false_5, %251 : i1 loc(#loc14)
    %253 = arith.xori %252, %true_0 : i1 loc(#loc14)
    %254 = arith.andi %107, %253 : i1 loc(#loc14)
    %255 = arith.xori %254, %true_0 : i1
    %256 = arith.andi %225, %243 : i1
    %257 = arith.ori %255, %256 : i1
    %258 = arith.andi %254, %257 : i1
    %259 = arith.andi %107, %252 : i1
    %260 = arith.xori %259, %true_0 : i1
    %261 = arith.andi %225, %243 : i1
    %262 = arith.ori %260, %261 : i1
    %263 = arith.andi %262, %true : i1
    %264 = arith.andi %259, %263 : i1
    %265 = arith.ori %258, %264 : i1
    %266 = arith.andi %249, %265 : i1
    %267 = arith.ori %247, %266 : i1
    %268 = arith.andi %246, %267 : i1
    %269 = arith.xori %107, %true_0 : i1 loc(#loc15)
    %270 = arith.xori %107, %true_0 : i1
    %271 = arith.ori %270, %249 : i1
    %272 = acir.extract %0 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %273 = acir.extract %272 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc16)
    %274 = arith.select %254, %false_5, %273 : i1 loc(#loc16)
    %275 = arith.xori %274, %true_0 : i1 loc(#loc16)
    %276 = arith.andi %107, %275 : i1 loc(#loc16)
    %277 = arith.xori %276, %true_0 : i1
    %278 = arith.andi %249, %265 : i1
    %279 = arith.ori %277, %278 : i1
    %280 = arith.andi %276, %279 : i1
    %281 = arith.andi %107, %274 : i1
    %282 = arith.xori %281, %true_0 : i1
    %283 = arith.andi %249, %265 : i1
    %284 = arith.ori %282, %283 : i1
    %285 = arith.andi %284, %true : i1
    %286 = arith.andi %281, %285 : i1
    %287 = arith.ori %280, %286 : i1
    %288 = arith.andi %271, %287 : i1
    %289 = arith.andi %107, %288 : i1
    %290 = arith.andi %214, %289 : i1
    %291 = arith.andi %107, %290 : i1
    %292 = acir.extract %0 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc17)
    %293 = acir.extract %292 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc17)
    %294 = arith.select %276, %false_5, %293 : i1 loc(#loc17)
    %295 = arith.andi %291, %294 : i1 loc(#loc17)
    %296 = arith.andi %213, %295 : i1 loc(#loc17)
    %297 = arith.ori %269, %296 : i1 loc(#loc17)
    %298 = arith.andi %268, %297 : i1 loc(#loc17)
    %299 = arith.xori %107, %true_0 : i1 loc(#loc18)
    %300 = arith.xori %107, %true_0 : i1 loc(#loc15)
    %301 = arith.andi %213, %295 : i1 loc(#loc15)
    %302 = arith.ori %300, %301 : i1 loc(#loc15)
    %303 = arith.andi %302, %true : i1 loc(#loc18)
    %304 = arith.ori %299, %303 : i1 loc(#loc18)
    %305 = arith.andi %298, %304 : i1 loc(#loc18)
    %306 = arith.xori %107, %true_0 : i1 loc(#loc19)
    %307 = arith.ori %306, %302 : i1 loc(#loc19)
    %308 = arith.andi %305, %307 : i1 loc(#loc19)
    %309 = arith.andi %202, %true : i1 loc(#loc22)
    %310 = arith.andi %108, %309 : i1 loc(#loc22)
    %311 = arith.andi %310, %195 : i1 loc(#loc22)
    %312 = arith.andi %113, %311 : i1 loc(#loc22)
    %313 = arith.andi %108, %312 : i1 loc(#loc22)
    %314 = arith.andi %302, %true : i1 loc(#loc22)
    %315 = arith.andi %107, %314 : i1 loc(#loc22)
    %316 = arith.andi %315, %295 : i1 loc(#loc22)
    %317 = arith.andi %213, %316 : i1 loc(#loc22)
    %318 = arith.andi %107, %317 : i1 loc(#loc22)
    %319 = arith.ori %313, %318 : i1 loc(#loc22)
    %320 = arith.xori %319, %true_0 : i1 loc(#loc22)
    %321 = acir.current {aspect = "count", resource = "top.scheduler.add_requests"} : i32 loc(#loc23)
    %322 = acir.current {aspect = "storage", resource = "top.scheduler.add_requests"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc24)
    %c0_i32_6 = arith.constant 0 : i32 loc(#loc24)
    %323 = acir.extract %322[%c0_i32_6] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc24)
    %c0_i32_7 = arith.constant 0 : i32 loc(#loc24)
    %324 = arith.cmpi ne, %321, %c0_i32_7 : i32 loc(#loc24)
    %325 = arith.andi %324, %324 : i1 loc(#loc24)
    %c0_i32_8 = arith.constant 0 : i32 loc(#loc24)
    %326 = arith.cmpi eq, %321, %c0_i32_8 : i32 loc(#loc24)
    %327 = arith.andi %325, %true : i1 loc(#loc24)
    %328 = arith.xori %326, %true_0 : i1 loc(#loc24)
    %329 = arith.andi %327, %328 : i1 loc(#loc24)
    %330 = arith.andi %324, %324 : i1 loc(#loc25)
    %331 = arith.andi %324, %324 : i1 loc(#loc26)
    %332 = arith.andi %330, %331 : i1 loc(#loc26)
    %333 = arith.xori %332, %true_0 : i1 loc(#loc26)
    %334 = acir.current {aspect = "count", resource = "top.adder.out"} : i32 loc(#loc27)
    %c0_i32_9 = arith.constant 0 : i32 loc(#loc28)
    %335 = arith.cmpi eq, %334, %c0_i32_9 : i32 loc(#loc28)
    %336 = arith.andi %true, %true : i1 loc(#loc28)
    %337 = acir.unary %335 "not" {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1 loc(#loc29)
    %338 = arith.xori %337, %true_0 : i1 loc(#loc29)
    %339 = arith.xori %338, %true_0 : i1 loc(#loc29)
    %340 = arith.xori %338, %true_0 : i1
    %341 = arith.ori %340, %true : i1
    %342 = acir.current {aspect = "storage", resource = "top.multiplier.out"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc30)
    %c0_i32_10 = arith.constant 0 : i32 loc(#loc30)
    %343 = acir.extract %342[%c0_i32_10] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc30)
    %344 = acir.current {aspect = "count", resource = "top.multiplier.out"} : i32 loc(#loc30)
    %c0_i32_11 = arith.constant 0 : i32 loc(#loc30)
    %345 = arith.cmpi ne, %344, %c0_i32_11 : i32 loc(#loc30)
    %346 = arith.andi %341, %345 : i1 loc(#loc30)
    %347 = arith.ori %339, %346 : i1 loc(#loc30)
    %348 = arith.andi %336, %347 : i1 loc(#loc30)
    %349 = arith.xori %338, %true_0 : i1 loc(#loc30)
    %350 = arith.andi %341, %345 : i1 loc(#loc30)
    %351 = arith.ori %349, %350 : i1 loc(#loc30)
    %352 = arith.andi %348, %351 : i1 loc(#loc30)
    %353 = arith.xori %338, %true_0 : i1 loc(#loc30)
    %c0_i32_12 = arith.constant 0 : i32 loc(#loc30)
    %354 = arith.cmpi eq, %344, %c0_i32_12 : i32 loc(#loc30)
    %355 = arith.andi %341, %true : i1 loc(#loc30)
    %356 = arith.ori %353, %355 : i1 loc(#loc30)
    %357 = arith.andi %352, %356 : i1 loc(#loc30)
    %358 = arith.xori %338, %true_0 : i1 loc(#loc30)
    %359 = arith.xori %354, %true_0 : i1 loc(#loc30)
    %360 = arith.andi %341, %359 : i1 loc(#loc30)
    %361 = arith.ori %358, %360 : i1 loc(#loc30)
    %362 = arith.andi %357, %361 : i1 loc(#loc30)
    %363 = arith.xori %337, %true_0 : i1 loc(#loc31)
    %364 = arith.xori %337, %true_0 : i1
    %365 = arith.ori %364, %true : i1
    %366 = acir.current {aspect = "storage", resource = "top.adder.out"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc31)
    %c0_i32_13 = arith.constant 0 : i32 loc(#loc31)
    %367 = acir.extract %366[%c0_i32_13] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc31)
    %c0_i32_14 = arith.constant 0 : i32 loc(#loc31)
    %368 = arith.cmpi ne, %334, %c0_i32_14 : i32 loc(#loc31)
    %369 = arith.andi %365, %368 : i1 loc(#loc31)
    %370 = arith.ori %363, %369 : i1 loc(#loc31)
    %371 = arith.andi %362, %370 : i1 loc(#loc31)
    %372 = arith.xori %337, %true_0 : i1 loc(#loc31)
    %373 = arith.andi %365, %368 : i1 loc(#loc31)
    %374 = arith.ori %372, %373 : i1 loc(#loc31)
    %375 = arith.andi %371, %374 : i1 loc(#loc31)
    %376 = arith.xori %337, %true_0 : i1 loc(#loc31)
    %c0_i32_15 = arith.constant 0 : i32 loc(#loc31)
    %377 = arith.cmpi eq, %334, %c0_i32_15 : i32 loc(#loc31)
    %378 = arith.andi %365, %true : i1 loc(#loc31)
    %379 = arith.ori %376, %378 : i1 loc(#loc31)
    %380 = arith.andi %375, %379 : i1 loc(#loc31)
    %381 = arith.xori %337, %true_0 : i1 loc(#loc31)
    %382 = arith.xori %377, %true_0 : i1 loc(#loc31)
    %383 = arith.andi %365, %382 : i1 loc(#loc31)
    %384 = arith.ori %381, %383 : i1 loc(#loc31)
    %385 = arith.andi %380, %384 : i1 loc(#loc31)
    %386 = arith.andi %341, %345 : i1
    %387 = arith.andi %338, %386 : i1
    %388 = arith.andi %365, %368 : i1
    %389 = arith.andi %337, %388 : i1
    %390 = arith.ori %387, %389 : i1
    %391 = arith.andi %385, %390 : i1
    %392 = arith.select %338, %343, %367 : !acir.struct<"src_payloads_25ebcaa7.Completion">
    %393 = acir.extract %392 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %394 = arith.subi %393, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc32)
    %395 = arith.andi %394, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc32)
    %c0_i32_16 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc33)
    %396 = arith.cmpi eq, %395, %c0_i32_16 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc33)
    %397 = arith.xori %396, %true_0 : i1 loc(#loc33)
    %398 = arith.xori %397, %true_0 : i1 loc(#loc33)
    %399 = arith.xori %397, %true_0 : i1
    %400 = arith.ori %399, %390 : i1
    %401 = arith.andi %400, %390 : i1
    %402 = arith.ori %398, %401 : i1
    %403 = arith.andi %391, %402 : i1
    %404 = arith.cmpi eq, %395, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc34)
    %405 = arith.xori %404, %true_0 : i1 loc(#loc34)
    %406 = arith.andi %397, %405 : i1 loc(#loc34)
    %407 = arith.xori %406, %true_0 : i1 loc(#loc34)
    %408 = arith.xori %406, %true_0 : i1
    %409 = arith.andi %400, %390 : i1
    %410 = arith.ori %408, %409 : i1
    %411 = arith.andi %410, %390 : i1
    %412 = arith.ori %407, %411 : i1
    %413 = arith.andi %403, %412 : i1
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %414 = arith.cmpi eq, %395, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32
    %415 = arith.xori %414, %true_0 : i1
    %416 = arith.andi %406, %415 : i1
    %417 = arith.xori %416, %true_0 : i1
    %418 = arith.xori %416, %true_0 : i1 loc(#loc35)
    %419 = arith.xori %416, %true_0 : i1
    %420 = arith.andi %410, %390 : i1
    %421 = arith.ori %419, %420 : i1
    %422 = arith.ori %418, %421 : i1
    %423 = acir.current {aspect = "value", resource = "top.rob.slot3.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc36)
    %424 = arith.andi %422, %true : i1 loc(#loc36)
    %425 = arith.ori %417, %424 : i1 loc(#loc36)
    %426 = arith.andi %413, %425 : i1 loc(#loc36)
    %427 = arith.xori %416, %true_0 : i1
    %428 = arith.andi %422, %true : i1
    %429 = arith.ori %427, %428 : i1
    %430 = arith.andi %426, %429 : i1
    %431 = arith.xori %416, %true_0 : i1
    %432 = arith.xori %416, %true_0 : i1
    %433 = arith.ori %432, %422 : i1
    %434 = acir.extract %423 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc37)
    %435 = arith.xori %434, %true_0 : i1 loc(#loc37)
    %436 = arith.andi %416, %435 : i1 loc(#loc37)
    %437 = arith.xori %436, %true_0 : i1
    %438 = arith.andi %422, %true : i1
    %439 = arith.ori %437, %438 : i1
    %440 = arith.andi %436, %439 : i1
    %441 = arith.andi %416, %434 : i1
    %442 = arith.xori %441, %true_0 : i1
    %443 = arith.andi %422, %true : i1
    %444 = arith.ori %442, %443 : i1
    %445 = arith.andi %444, %true : i1
    %446 = arith.andi %441, %445 : i1
    %447 = arith.ori %440, %446 : i1
    %448 = arith.andi %433, %447 : i1
    %449 = arith.ori %431, %448 : i1
    %450 = arith.andi %430, %449 : i1
    %451 = arith.xori %416, %true_0 : i1
    %452 = arith.xori %416, %true_0 : i1
    %453 = arith.ori %452, %433 : i1
    %false_17 = arith.constant {acir.source_type = "bool"} false
    %454 = acir.extract %423 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc38)
    %455 = acir.unary %454 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc38)
    %456 = arith.select %436, %false_17, %455 : i1 loc(#loc38)
    %457 = arith.xori %456, %true_0 : i1 loc(#loc38)
    %458 = arith.andi %416, %457 : i1 loc(#loc38)
    %459 = arith.xori %458, %true_0 : i1
    %460 = arith.andi %433, %447 : i1
    %461 = arith.ori %459, %460 : i1
    %462 = arith.andi %458, %461 : i1
    %463 = arith.andi %416, %456 : i1
    %464 = arith.xori %463, %true_0 : i1
    %465 = arith.andi %433, %447 : i1
    %466 = arith.ori %464, %465 : i1
    %467 = arith.andi %421, %390 : i1 loc(#loc35)
    %468 = arith.andi %416, %467 : i1 loc(#loc35)
    %469 = arith.andi %true, %468 : i1 loc(#loc35)
    %470 = arith.andi %466, %469 : i1 loc(#loc35)
    %471 = arith.andi %463, %470 : i1 loc(#loc35)
    %472 = arith.ori %462, %471 : i1 loc(#loc35)
    %473 = arith.andi %453, %472 : i1 loc(#loc35)
    %474 = arith.ori %451, %473 : i1 loc(#loc35)
    %475 = arith.andi %450, %474 : i1 loc(#loc35)
    %476 = acir.extract %423 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc39)
    %477 = arith.cmpi eq, %476, %393 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc39)
    %478 = arith.select %458, %false_17, %477 : i1 loc(#loc39)
    %479 = arith.andi %416, %478 : i1 loc(#loc39)
    %480 = arith.xori %479, %true_0 : i1 loc(#loc39)
    %481 = arith.xori %479, %true_0 : i1
    %482 = arith.andi %453, %472 : i1
    %483 = arith.ori %481, %482 : i1
    %484 = arith.andi %421, %390 : i1 loc(#loc35)
    %485 = arith.andi %416, %484 : i1 loc(#loc35)
    %486 = arith.andi %true, %485 : i1 loc(#loc35)
    %487 = arith.andi %483, %486 : i1 loc(#loc35)
    %488 = arith.ori %480, %487 : i1 loc(#loc35)
    %489 = arith.andi %475, %488 : i1 loc(#loc35)
    %490 = arith.andi %406, %414 : i1
    %491 = arith.xori %490, %true_0 : i1
    %492 = arith.xori %490, %true_0 : i1 loc(#loc40)
    %493 = arith.xori %490, %true_0 : i1
    %494 = arith.andi %410, %390 : i1
    %495 = arith.ori %493, %494 : i1
    %496 = arith.ori %492, %495 : i1
    %497 = acir.current {aspect = "value", resource = "top.rob.slot2.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc36)
    %498 = arith.andi %496, %true : i1 loc(#loc36)
    %499 = arith.ori %491, %498 : i1 loc(#loc36)
    %500 = arith.andi %489, %499 : i1 loc(#loc36)
    %501 = arith.xori %490, %true_0 : i1
    %502 = arith.andi %496, %true : i1
    %503 = arith.ori %501, %502 : i1
    %504 = arith.andi %500, %503 : i1
    %505 = arith.xori %490, %true_0 : i1
    %506 = arith.xori %490, %true_0 : i1
    %507 = arith.ori %506, %496 : i1
    %508 = acir.extract %497 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc37)
    %509 = arith.xori %508, %true_0 : i1 loc(#loc37)
    %510 = arith.andi %490, %509 : i1 loc(#loc37)
    %511 = arith.xori %510, %true_0 : i1
    %512 = arith.andi %496, %true : i1
    %513 = arith.ori %511, %512 : i1
    %514 = arith.andi %510, %513 : i1
    %515 = arith.andi %490, %508 : i1
    %516 = arith.xori %515, %true_0 : i1
    %517 = arith.andi %496, %true : i1
    %518 = arith.ori %516, %517 : i1
    %519 = arith.andi %518, %true : i1
    %520 = arith.andi %515, %519 : i1
    %521 = arith.ori %514, %520 : i1
    %522 = arith.andi %507, %521 : i1
    %523 = arith.ori %505, %522 : i1
    %524 = arith.andi %504, %523 : i1
    %525 = arith.xori %490, %true_0 : i1
    %526 = arith.xori %490, %true_0 : i1
    %527 = arith.ori %526, %507 : i1
    %false_18 = arith.constant {acir.source_type = "bool"} false
    %528 = acir.extract %497 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc38)
    %529 = acir.unary %528 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc38)
    %530 = arith.select %510, %false_18, %529 : i1 loc(#loc38)
    %531 = arith.xori %530, %true_0 : i1 loc(#loc38)
    %532 = arith.andi %490, %531 : i1 loc(#loc38)
    %533 = arith.xori %532, %true_0 : i1
    %534 = arith.andi %507, %521 : i1
    %535 = arith.ori %533, %534 : i1
    %536 = arith.andi %532, %535 : i1
    %537 = arith.andi %490, %530 : i1
    %538 = arith.xori %537, %true_0 : i1
    %539 = arith.andi %507, %521 : i1
    %540 = arith.ori %538, %539 : i1
    %541 = arith.andi %495, %390 : i1 loc(#loc40)
    %542 = arith.andi %490, %541 : i1 loc(#loc40)
    %543 = arith.andi %true, %542 : i1 loc(#loc40)
    %544 = arith.andi %540, %543 : i1 loc(#loc40)
    %545 = arith.andi %537, %544 : i1 loc(#loc40)
    %546 = arith.ori %536, %545 : i1 loc(#loc40)
    %547 = arith.andi %527, %546 : i1 loc(#loc40)
    %548 = arith.ori %525, %547 : i1 loc(#loc40)
    %549 = arith.andi %524, %548 : i1 loc(#loc40)
    %550 = acir.extract %497 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc39)
    %551 = arith.cmpi eq, %550, %393 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc39)
    %552 = arith.select %532, %false_18, %551 : i1 loc(#loc39)
    %553 = arith.andi %490, %552 : i1 loc(#loc39)
    %554 = arith.xori %553, %true_0 : i1 loc(#loc39)
    %555 = arith.xori %553, %true_0 : i1
    %556 = arith.andi %527, %546 : i1
    %557 = arith.ori %555, %556 : i1
    %558 = arith.andi %495, %390 : i1 loc(#loc40)
    %559 = arith.andi %490, %558 : i1 loc(#loc40)
    %560 = arith.andi %true, %559 : i1 loc(#loc40)
    %561 = arith.andi %557, %560 : i1 loc(#loc40)
    %562 = arith.ori %554, %561 : i1 loc(#loc40)
    %563 = arith.andi %549, %562 : i1 loc(#loc40)
    %564 = arith.andi %397, %404 : i1
    %565 = arith.xori %564, %true_0 : i1
    %566 = arith.xori %564, %true_0 : i1 loc(#loc41)
    %567 = arith.xori %564, %true_0 : i1
    %568 = arith.andi %400, %390 : i1
    %569 = arith.ori %567, %568 : i1
    %570 = arith.ori %566, %569 : i1
    %571 = acir.current {aspect = "value", resource = "top.rob.slot1.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc36)
    %572 = arith.andi %570, %true : i1 loc(#loc36)
    %573 = arith.ori %565, %572 : i1 loc(#loc36)
    %574 = arith.andi %563, %573 : i1 loc(#loc36)
    %575 = arith.xori %564, %true_0 : i1
    %576 = arith.andi %570, %true : i1
    %577 = arith.ori %575, %576 : i1
    %578 = arith.andi %574, %577 : i1
    %579 = arith.xori %564, %true_0 : i1
    %580 = arith.xori %564, %true_0 : i1
    %581 = arith.ori %580, %570 : i1
    %582 = acir.extract %571 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc37)
    %583 = arith.xori %582, %true_0 : i1 loc(#loc37)
    %584 = arith.andi %564, %583 : i1 loc(#loc37)
    %585 = arith.xori %584, %true_0 : i1
    %586 = arith.andi %570, %true : i1
    %587 = arith.ori %585, %586 : i1
    %588 = arith.andi %584, %587 : i1
    %589 = arith.andi %564, %582 : i1
    %590 = arith.xori %589, %true_0 : i1
    %591 = arith.andi %570, %true : i1
    %592 = arith.ori %590, %591 : i1
    %593 = arith.andi %592, %true : i1
    %594 = arith.andi %589, %593 : i1
    %595 = arith.ori %588, %594 : i1
    %596 = arith.andi %581, %595 : i1
    %597 = arith.ori %579, %596 : i1
    %598 = arith.andi %578, %597 : i1
    %599 = arith.xori %564, %true_0 : i1
    %600 = arith.xori %564, %true_0 : i1
    %601 = arith.ori %600, %581 : i1
    %false_19 = arith.constant {acir.source_type = "bool"} false
    %602 = acir.extract %571 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc38)
    %603 = acir.unary %602 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc38)
    %604 = arith.select %584, %false_19, %603 : i1 loc(#loc38)
    %605 = arith.xori %604, %true_0 : i1 loc(#loc38)
    %606 = arith.andi %564, %605 : i1 loc(#loc38)
    %607 = arith.xori %606, %true_0 : i1
    %608 = arith.andi %581, %595 : i1
    %609 = arith.ori %607, %608 : i1
    %610 = arith.andi %606, %609 : i1
    %611 = arith.andi %564, %604 : i1
    %612 = arith.xori %611, %true_0 : i1
    %613 = arith.andi %581, %595 : i1
    %614 = arith.ori %612, %613 : i1
    %615 = arith.andi %569, %390 : i1 loc(#loc41)
    %616 = arith.andi %564, %615 : i1 loc(#loc41)
    %617 = arith.andi %true, %616 : i1 loc(#loc41)
    %618 = arith.andi %614, %617 : i1 loc(#loc41)
    %619 = arith.andi %611, %618 : i1 loc(#loc41)
    %620 = arith.ori %610, %619 : i1 loc(#loc41)
    %621 = arith.andi %601, %620 : i1 loc(#loc41)
    %622 = arith.ori %599, %621 : i1 loc(#loc41)
    %623 = arith.andi %598, %622 : i1 loc(#loc41)
    %624 = acir.extract %571 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc39)
    %625 = arith.cmpi eq, %624, %393 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc39)
    %626 = arith.select %606, %false_19, %625 : i1 loc(#loc39)
    %627 = arith.andi %564, %626 : i1 loc(#loc39)
    %628 = arith.xori %627, %true_0 : i1 loc(#loc39)
    %629 = arith.xori %627, %true_0 : i1
    %630 = arith.andi %601, %620 : i1
    %631 = arith.ori %629, %630 : i1
    %632 = arith.andi %569, %390 : i1 loc(#loc41)
    %633 = arith.andi %564, %632 : i1 loc(#loc41)
    %634 = arith.andi %true, %633 : i1 loc(#loc41)
    %635 = arith.andi %631, %634 : i1 loc(#loc41)
    %636 = arith.ori %628, %635 : i1 loc(#loc41)
    %637 = arith.andi %623, %636 : i1 loc(#loc41)
    %638 = arith.xori %396, %true_0 : i1 loc(#loc36)
    %639 = arith.xori %396, %true_0 : i1 loc(#loc42)
    %640 = arith.xori %396, %true_0 : i1
    %641 = arith.ori %640, %390 : i1
    %642 = arith.ori %639, %641 : i1
    %643 = acir.current {aspect = "value", resource = "top.rob.slot0.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc36)
    %644 = arith.andi %642, %true : i1 loc(#loc36)
    %645 = arith.ori %638, %644 : i1 loc(#loc36)
    %646 = arith.andi %637, %645 : i1 loc(#loc36)
    %647 = arith.xori %396, %true_0 : i1
    %648 = arith.andi %642, %true : i1
    %649 = arith.ori %647, %648 : i1
    %650 = arith.andi %646, %649 : i1
    %651 = arith.xori %396, %true_0 : i1
    %652 = arith.xori %396, %true_0 : i1
    %653 = arith.ori %652, %642 : i1
    %654 = acir.extract %643 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc37)
    %655 = arith.xori %654, %true_0 : i1 loc(#loc37)
    %656 = arith.andi %396, %655 : i1 loc(#loc37)
    %657 = arith.xori %656, %true_0 : i1
    %658 = arith.andi %642, %true : i1
    %659 = arith.ori %657, %658 : i1
    %660 = arith.andi %656, %659 : i1
    %661 = arith.andi %396, %654 : i1
    %662 = arith.xori %661, %true_0 : i1
    %663 = arith.andi %642, %true : i1
    %664 = arith.ori %662, %663 : i1
    %665 = arith.andi %664, %true : i1
    %666 = arith.andi %661, %665 : i1
    %667 = arith.ori %660, %666 : i1
    %668 = arith.andi %653, %667 : i1
    %669 = arith.ori %651, %668 : i1
    %670 = arith.andi %650, %669 : i1
    %671 = arith.xori %396, %true_0 : i1
    %672 = arith.xori %396, %true_0 : i1
    %673 = arith.ori %672, %653 : i1
    %false_20 = arith.constant {acir.source_type = "bool"} false
    %674 = acir.extract %643 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc38)
    %675 = acir.unary %674 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc38)
    %676 = arith.select %656, %false_20, %675 : i1 loc(#loc38)
    %677 = arith.xori %676, %true_0 : i1 loc(#loc38)
    %678 = arith.andi %396, %677 : i1 loc(#loc38)
    %679 = arith.xori %678, %true_0 : i1
    %680 = arith.andi %653, %667 : i1
    %681 = arith.ori %679, %680 : i1
    %682 = arith.andi %678, %681 : i1
    %683 = arith.andi %396, %676 : i1
    %684 = arith.xori %683, %true_0 : i1
    %685 = arith.andi %653, %667 : i1
    %686 = arith.ori %684, %685 : i1
    %687 = arith.andi %641, %390 : i1 loc(#loc42)
    %688 = arith.andi %396, %687 : i1 loc(#loc42)
    %689 = arith.andi %true, %688 : i1 loc(#loc42)
    %690 = arith.andi %686, %689 : i1 loc(#loc42)
    %691 = arith.andi %683, %690 : i1 loc(#loc42)
    %692 = arith.ori %682, %691 : i1 loc(#loc42)
    %693 = arith.andi %673, %692 : i1 loc(#loc42)
    %694 = arith.ori %671, %693 : i1 loc(#loc42)
    %695 = arith.andi %670, %694 : i1 loc(#loc42)
    %696 = acir.extract %643 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc39)
    %697 = arith.cmpi eq, %696, %393 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc39)
    %698 = arith.select %678, %false_20, %697 : i1 loc(#loc39)
    %699 = arith.andi %396, %698 : i1 loc(#loc39)
    %700 = arith.xori %699, %true_0 : i1 loc(#loc39)
    %701 = arith.xori %699, %true_0 : i1
    %702 = arith.andi %673, %692 : i1
    %703 = arith.ori %701, %702 : i1
    %704 = arith.andi %641, %390 : i1 loc(#loc42)
    %705 = arith.andi %396, %704 : i1 loc(#loc42)
    %706 = arith.andi %true, %705 : i1 loc(#loc42)
    %707 = arith.andi %703, %706 : i1 loc(#loc42)
    %708 = arith.ori %700, %707 : i1 loc(#loc42)
    %709 = arith.andi %695, %708 : i1 loc(#loc42)
    %710 = arith.andi %709, %true : i1 loc(#loc43)
    %711 = arith.andi %710, %true : i1 loc(#loc43)
    %712 = acir.extract %0 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc44)
    %713 = arith.xori %712, %true_0 : i1 loc(#loc44)
    %714 = arith.xori %712, %true_0 : i1
    %715 = arith.ori %714, %true : i1
    %716 = arith.andi %715, %true : i1
    %717 = arith.ori %713, %716 : i1
    %718 = arith.andi %711, %717 : i1
    %719 = arith.xori %712, %true_0 : i1
    %720 = arith.xori %712, %true_0 : i1
    %721 = arith.ori %720, %715 : i1
    %722 = acir.extract %0 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc45)
    %723 = acir.extract %722 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc45)
    %724 = acir.unary %723 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc45)
    %725 = arith.xori %724, %true_0 : i1 loc(#loc45)
    %726 = arith.andi %712, %725 : i1 loc(#loc45)
    %727 = arith.xori %726, %true_0 : i1
    %728 = arith.andi %715, %true : i1
    %729 = arith.ori %727, %728 : i1
    %730 = arith.andi %726, %729 : i1
    %731 = arith.andi %712, %724 : i1
    %732 = arith.xori %731, %true_0 : i1
    %733 = arith.andi %715, %true : i1
    %734 = arith.ori %732, %733 : i1
    %735 = arith.andi %true, %390 : i1 loc(#loc46)
    %736 = arith.andi %734, %735 : i1 loc(#loc46)
    %737 = arith.andi %731, %736 : i1 loc(#loc46)
    %738 = arith.ori %730, %737 : i1 loc(#loc46)
    %739 = arith.andi %721, %738 : i1 loc(#loc46)
    %740 = arith.ori %719, %739 : i1 loc(#loc46)
    %741 = arith.andi %718, %740 : i1 loc(#loc46)
    %742 = arith.xori %712, %true_0 : i1
    %743 = arith.xori %712, %true_0 : i1
    %744 = arith.ori %743, %721 : i1
    %false_21 = arith.constant {acir.source_type = "bool"} false
    %745 = acir.extract %0 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc46)
    %746 = acir.extract %745 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc46)
    %747 = acir.extract %392 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc47)
    %748 = arith.cmpi eq, %746, %747 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc47)
    %749 = arith.select %726, %false_21, %748 : i1 loc(#loc47)
    %750 = arith.xori %749, %true_0 : i1 loc(#loc47)
    %751 = arith.andi %712, %750 : i1 loc(#loc47)
    %752 = arith.xori %751, %true_0 : i1
    %753 = arith.andi %721, %738 : i1
    %754 = arith.ori %752, %753 : i1
    %755 = arith.andi %754, %true : i1
    %756 = arith.andi %751, %755 : i1
    %757 = arith.andi %712, %749 : i1
    %758 = arith.xori %757, %true_0 : i1
    %759 = arith.andi %721, %738 : i1
    %760 = arith.ori %758, %759 : i1
    %761 = arith.andi %390, %390 : i1 loc(#loc48)
    %762 = arith.andi %true, %761 : i1 loc(#loc48)
    %763 = arith.andi %760, %762 : i1 loc(#loc48)
    %764 = arith.andi %757, %763 : i1 loc(#loc48)
    %765 = arith.ori %756, %764 : i1 loc(#loc48)
    %766 = arith.andi %744, %765 : i1 loc(#loc48)
    %767 = arith.ori %742, %766 : i1 loc(#loc48)
    %768 = arith.andi %741, %767 : i1 loc(#loc48)
    %769 = arith.xori %712, %true_0 : i1
    %770 = arith.xori %712, %true_0 : i1
    %771 = arith.ori %770, %744 : i1
    %true_22 = arith.constant {acir.source_type = "bool"} true
    %772 = acir.extract %392 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc49)
    %773 = acir.aggregate(%true_22, %747, %772) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc49)
    %774 = acir.update %0, %773 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc49)
    %775 = arith.select %751, %0, %774 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc49)
    %776 = acir.extract %775 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc49)
    %777 = acir.extract %776 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc49)
    %778 = acir.unary %777 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc49)
    %779 = arith.xori %778, %true_0 : i1 loc(#loc49)
    %780 = arith.andi %712, %779 : i1 loc(#loc49)
    %781 = arith.xori %780, %true_0 : i1
    %782 = arith.andi %744, %765 : i1
    %783 = arith.ori %781, %782 : i1
    %784 = arith.andi %780, %783 : i1
    %785 = arith.andi %712, %778 : i1
    %786 = arith.xori %785, %true_0 : i1
    %787 = arith.andi %744, %765 : i1
    %788 = arith.ori %786, %787 : i1
    %789 = arith.andi %765, %390 : i1 loc(#loc50)
    %790 = arith.andi %788, %789 : i1 loc(#loc50)
    %791 = arith.andi %785, %790 : i1 loc(#loc50)
    %792 = arith.ori %784, %791 : i1 loc(#loc50)
    %793 = arith.andi %771, %792 : i1 loc(#loc50)
    %794 = arith.ori %769, %793 : i1 loc(#loc50)
    %795 = arith.andi %768, %794 : i1 loc(#loc50)
    %796 = arith.xori %712, %true_0 : i1
    %797 = arith.xori %796, %true_0 : i1
    %798 = arith.ori %797, %true : i1
    %799 = arith.andi %796, %798 : i1
    %800 = arith.xori %712, %true_0 : i1
    %801 = arith.ori %800, %771 : i1
    %802 = arith.andi %792, %738 : i1
    %803 = arith.andi %801, %802 : i1
    %804 = arith.andi %712, %803 : i1
    %805 = arith.ori %799, %804 : i1
    %806 = arith.andi %795, %805 : i1
    %807 = acir.extract %775 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc50)
    %808 = acir.extract %807 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc50)
    %809 = arith.cmpi eq, %808, %747 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc50)
    %810 = arith.select %780, %false_21, %809 : i1 loc(#loc50)
    %811 = arith.select %810, %true_22, %749 : i1 loc(#loc50)
    %812 = arith.select %796, %false_21, %811 : i1 loc(#loc50)
    %813 = arith.xori %812, %true_0 : i1 loc(#loc50)
    %814 = arith.xori %812, %true_0 : i1
    %815 = arith.ori %814, %805 : i1
    %816 = arith.andi %798, %true : i1
    %817 = arith.andi %796, %816 : i1
    %818 = arith.xori %810, %true_0 : i1
    %819 = arith.andi %712, %818 : i1
    %820 = arith.xori %819, %true_0 : i1
    %821 = arith.andi %771, %792 : i1
    %822 = arith.ori %820, %821 : i1
    %823 = arith.andi %822, %765 : i1
    %824 = arith.andi %819, %823 : i1
    %825 = arith.andi %712, %810 : i1
    %826 = arith.xori %825, %true_0 : i1
    %827 = arith.andi %771, %792 : i1
    %828 = arith.ori %826, %827 : i1
    %829 = arith.andi %390, %390 : i1 loc(#loc51)
    %830 = arith.andi %765, %829 : i1 loc(#loc51)
    %831 = arith.andi %828, %830 : i1 loc(#loc51)
    %832 = arith.andi %825, %831 : i1 loc(#loc51)
    %833 = arith.ori %824, %832 : i1 loc(#loc51)
    %834 = arith.andi %801, %833 : i1 loc(#loc51)
    %835 = arith.andi %712, %834 : i1 loc(#loc51)
    %836 = arith.ori %817, %835 : i1 loc(#loc51)
    %837 = arith.andi %815, %836 : i1 loc(#loc51)
    %838 = arith.ori %813, %837 : i1 loc(#loc51)
    %839 = arith.andi %806, %838 : i1 loc(#loc51)
    %840 = arith.andi %839, %true : i1 loc(#loc43)
    %841 = arith.andi %840, %true : i1 loc(#loc43)
    %842 = acir.extract %46 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc44)
    %843 = arith.xori %842, %true_0 : i1 loc(#loc44)
    %844 = arith.xori %842, %true_0 : i1
    %845 = arith.ori %844, %true : i1
    %846 = arith.andi %845, %true : i1
    %847 = arith.ori %843, %846 : i1
    %848 = arith.andi %841, %847 : i1
    %849 = arith.xori %842, %true_0 : i1
    %850 = arith.xori %842, %true_0 : i1
    %851 = arith.ori %850, %845 : i1
    %852 = acir.extract %46 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc45)
    %853 = acir.extract %852 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc45)
    %854 = acir.unary %853 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc45)
    %855 = arith.xori %854, %true_0 : i1 loc(#loc45)
    %856 = arith.andi %842, %855 : i1 loc(#loc45)
    %857 = arith.xori %856, %true_0 : i1
    %858 = arith.andi %845, %true : i1
    %859 = arith.ori %857, %858 : i1
    %860 = arith.andi %856, %859 : i1
    %861 = arith.andi %842, %854 : i1
    %862 = arith.xori %861, %true_0 : i1
    %863 = arith.andi %845, %true : i1
    %864 = arith.ori %862, %863 : i1
    %865 = arith.andi %true, %390 : i1 loc(#loc46)
    %866 = arith.andi %864, %865 : i1 loc(#loc46)
    %867 = arith.andi %861, %866 : i1 loc(#loc46)
    %868 = arith.ori %860, %867 : i1 loc(#loc46)
    %869 = arith.andi %851, %868 : i1 loc(#loc46)
    %870 = arith.ori %849, %869 : i1 loc(#loc46)
    %871 = arith.andi %848, %870 : i1 loc(#loc46)
    %872 = arith.xori %842, %true_0 : i1
    %873 = arith.xori %842, %true_0 : i1
    %874 = arith.ori %873, %851 : i1
    %false_23 = arith.constant {acir.source_type = "bool"} false
    %875 = acir.extract %46 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc46)
    %876 = acir.extract %875 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc46)
    %877 = acir.extract %392 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc52)
    %878 = arith.cmpi eq, %876, %877 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc52)
    %879 = arith.select %856, %false_23, %878 : i1 loc(#loc52)
    %880 = arith.xori %879, %true_0 : i1 loc(#loc52)
    %881 = arith.andi %842, %880 : i1 loc(#loc52)
    %882 = arith.xori %881, %true_0 : i1
    %883 = arith.andi %851, %868 : i1
    %884 = arith.ori %882, %883 : i1
    %885 = arith.andi %884, %true : i1
    %886 = arith.andi %881, %885 : i1
    %887 = arith.andi %842, %879 : i1
    %888 = arith.xori %887, %true_0 : i1
    %889 = arith.andi %851, %868 : i1
    %890 = arith.ori %888, %889 : i1
    %891 = arith.andi %390, %390 : i1 loc(#loc48)
    %892 = arith.andi %true, %891 : i1 loc(#loc48)
    %893 = arith.andi %890, %892 : i1 loc(#loc48)
    %894 = arith.andi %887, %893 : i1 loc(#loc48)
    %895 = arith.ori %886, %894 : i1 loc(#loc48)
    %896 = arith.andi %874, %895 : i1 loc(#loc48)
    %897 = arith.ori %872, %896 : i1 loc(#loc48)
    %898 = arith.andi %871, %897 : i1 loc(#loc48)
    %899 = arith.xori %842, %true_0 : i1
    %900 = arith.xori %842, %true_0 : i1
    %901 = arith.ori %900, %874 : i1
    %true_24 = arith.constant {acir.source_type = "bool"} true
    %902 = acir.extract %392 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc53)
    %903 = acir.aggregate(%true_24, %877, %902) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc53)
    %904 = acir.update %46, %903 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc53)
    %905 = arith.select %881, %46, %904 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc53)
    %906 = acir.extract %905 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc53)
    %907 = acir.extract %906 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc53)
    %908 = acir.unary %907 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc53)
    %909 = arith.xori %908, %true_0 : i1 loc(#loc53)
    %910 = arith.andi %842, %909 : i1 loc(#loc53)
    %911 = arith.xori %910, %true_0 : i1
    %912 = arith.andi %874, %895 : i1
    %913 = arith.ori %911, %912 : i1
    %914 = arith.andi %910, %913 : i1
    %915 = arith.andi %842, %908 : i1
    %916 = arith.xori %915, %true_0 : i1
    %917 = arith.andi %874, %895 : i1
    %918 = arith.ori %916, %917 : i1
    %919 = arith.andi %895, %390 : i1 loc(#loc50)
    %920 = arith.andi %918, %919 : i1 loc(#loc50)
    %921 = arith.andi %915, %920 : i1 loc(#loc50)
    %922 = arith.ori %914, %921 : i1 loc(#loc50)
    %923 = arith.andi %901, %922 : i1 loc(#loc50)
    %924 = arith.ori %899, %923 : i1 loc(#loc50)
    %925 = arith.andi %898, %924 : i1 loc(#loc50)
    %926 = arith.xori %842, %true_0 : i1
    %927 = arith.xori %926, %true_0 : i1
    %928 = arith.ori %927, %true : i1
    %929 = arith.andi %926, %928 : i1
    %930 = arith.xori %842, %true_0 : i1
    %931 = arith.ori %930, %901 : i1
    %932 = arith.andi %922, %868 : i1
    %933 = arith.andi %931, %932 : i1
    %934 = arith.andi %842, %933 : i1
    %935 = arith.ori %929, %934 : i1
    %936 = arith.andi %925, %935 : i1
    %937 = acir.extract %905 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc50)
    %938 = acir.extract %937 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc50)
    %939 = arith.cmpi eq, %938, %877 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc50)
    %940 = arith.select %910, %false_23, %939 : i1 loc(#loc50)
    %941 = arith.select %940, %true_24, %879 : i1 loc(#loc50)
    %942 = arith.select %926, %false_23, %941 : i1 loc(#loc50)
    %943 = arith.xori %942, %true_0 : i1 loc(#loc50)
    %944 = arith.xori %942, %true_0 : i1
    %945 = arith.ori %944, %935 : i1
    %946 = arith.andi %928, %true : i1
    %947 = arith.andi %926, %946 : i1
    %948 = arith.xori %940, %true_0 : i1
    %949 = arith.andi %842, %948 : i1
    %950 = arith.xori %949, %true_0 : i1
    %951 = arith.andi %901, %922 : i1
    %952 = arith.ori %950, %951 : i1
    %953 = arith.andi %952, %895 : i1
    %954 = arith.andi %949, %953 : i1
    %955 = arith.andi %842, %940 : i1
    %956 = arith.xori %955, %true_0 : i1
    %957 = arith.andi %901, %922 : i1
    %958 = arith.ori %956, %957 : i1
    %959 = arith.andi %390, %390 : i1 loc(#loc51)
    %960 = arith.andi %895, %959 : i1 loc(#loc51)
    %961 = arith.andi %958, %960 : i1 loc(#loc51)
    %962 = arith.andi %955, %961 : i1 loc(#loc51)
    %963 = arith.ori %954, %962 : i1 loc(#loc51)
    %964 = arith.andi %931, %963 : i1 loc(#loc51)
    %965 = arith.andi %842, %964 : i1 loc(#loc51)
    %966 = arith.ori %947, %965 : i1 loc(#loc51)
    %967 = arith.andi %945, %966 : i1 loc(#loc51)
    %968 = arith.ori %943, %967 : i1 loc(#loc51)
    %969 = arith.andi %936, %968 : i1 loc(#loc51)
    %970 = acir.fire "top.cdb.broadcast#5" %969 {successors = [2, 4, 8], txn = 5 : i64} loc(#loc51)
    %971 = arith.andi %970, %337 : i1 loc(#loc51)
    %c0_i32_25 = arith.constant 0 : i32 loc(#loc51)
    %c1_i32_26 = arith.constant 1 : i32 loc(#loc51)
    %c0_i32_27 = arith.constant 0 : i32 loc(#loc51)
    %972 = arith.select %971, %c1_i32_26, %c0_i32_27 : i32 loc(#loc51)
    %973 = arith.addi %c0_i32_25, %972 : i32 loc(#loc51)
    %974 = arith.subi %334, %973 : i32 loc(#loc27)
    %c1_i32_28 = arith.constant 1 : i32 loc(#loc51)
    %975 = arith.cmpi uge, %974, %c1_i32_28 : i32 loc(#loc51)
    %976 = arith.ori %333, %true : i1 loc(#loc51)
    %977 = arith.andi %329, %976 : i1 loc(#loc51)
    %978 = arith.xori %332, %true_0 : i1 loc(#loc27)
    %979 = arith.xori %975, %true_0 : i1 loc(#loc27)
    %980 = arith.ori %978, %979 : i1 loc(#loc27)
    %981 = arith.andi %977, %980 : i1 loc(#loc27)
    %982 = acir.fire "top.adder.execute#2" %981 {successors = [0], txn = 2 : i64} loc(#loc27)
    %c0_i32_29 = arith.constant 0 : i32 loc(#loc27)
    %c1_i32_30 = arith.constant 1 : i32 loc(#loc27)
    %c0_i32_31 = arith.constant 0 : i32 loc(#loc27)
    %983 = arith.select %982, %c1_i32_30, %c0_i32_31 : i32 loc(#loc27)
    %984 = arith.addi %c0_i32_29, %983 : i32 loc(#loc27)
    %985 = arith.subi %321, %984 : i32 loc(#loc23)
    %c1_i32_32 = arith.constant 1 : i32 loc(#loc27)
    %986 = arith.cmpi uge, %985, %c1_i32_32 : i32 loc(#loc27)
    %987 = arith.ori %320, %true : i1 loc(#loc27)
    %988 = arith.andi %308, %987 : i1 loc(#loc27)
    %989 = arith.xori %319, %true_0 : i1 loc(#loc23)
    %990 = arith.xori %986, %true_0 : i1 loc(#loc23)
    %991 = arith.ori %989, %990 : i1 loc(#loc23)
    %992 = arith.andi %988, %991 : i1 loc(#loc23)
    %993 = acir.fire "top.scheduler.issue_add#0" %992 {successors = [], txn = 0 : i64} loc(#loc23)
    %994 = arith.xori %9, %true_0 : i1
    %995 = acir.extract %0 {acir.id = "v52", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc54)
    %c1_i32_33 = arith.constant {acir.source_type = "u32"} 1 : i32
    %996 = arith.cmpi eq, %995, %c1_i32_33 {acir.id = "v54", acir.source_type = "bool"} : i32
    %997 = arith.select %994, %false_1, %996 : i1
    %998 = arith.xori %997, %true_0 : i1
    %999 = arith.xori %998, %true_0 : i1
    %1000 = arith.xori %994, %true_0 : i1
    %1001 = arith.ori %1000, %32 : i1
    %1002 = arith.andi %994, %1001 : i1
    %1003 = arith.xori %9, %true_0 : i1
    %1004 = arith.ori %1003, %32 : i1
    %1005 = arith.andi %1004, %true : i1
    %1006 = arith.andi %9, %1005 : i1
    %1007 = arith.ori %1002, %1006 : i1
    %1008 = arith.ori %999, %1007 : i1
    %1009 = arith.andi %998, %1008 : i1
    %1010 = arith.xori %997, %true_0 : i1
    %1011 = arith.xori %997, %true_0 : i1
    %1012 = arith.ori %1011, %1007 : i1
    %1013 = arith.ori %1010, %1012 : i1
    %1014 = arith.xori %55, %true_0 : i1
    %1015 = acir.extract %46 {acir.id = "v60", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc55)
    %1016 = arith.cmpi eq, %1015, %c1_i32_33 {acir.id = "v62", acir.source_type = "bool"} : i32 loc(#loc55)
    %1017 = arith.select %1014, %false_1, %1016 : i1 loc(#loc55)
    %1018 = acir.unary %1017 "not" {acir.id = "v82", acir.source_type = "bool"} : i1 -> i1 loc(#loc55)
    %1019 = arith.xori %1018, %true_0 : i1 loc(#loc55)
    %1020 = arith.andi %997, %1019 : i1 loc(#loc55)
    %1021 = arith.xori %1020, %true_0 : i1
    %1022 = arith.xori %1014, %true_0 : i1
    %1023 = arith.ori %1022, %80 : i1
    %1024 = arith.andi %1014, %1023 : i1
    %1025 = arith.xori %55, %true_0 : i1
    %1026 = arith.ori %1025, %80 : i1
    %1027 = arith.andi %1026, %true : i1
    %1028 = arith.andi %55, %1027 : i1
    %1029 = arith.ori %1024, %1028 : i1
    %1030 = arith.andi %1012, %1029 : i1
    %1031 = arith.ori %1021, %1030 : i1
    %1032 = arith.andi %true, %true : i1 loc(#loc56)
    %1033 = arith.andi %1031, %1032 : i1 loc(#loc56)
    %1034 = arith.andi %1020, %1033 : i1 loc(#loc56)
    %1035 = arith.andi %997, %1018 : i1
    %1036 = arith.xori %1035, %true_0 : i1
    %1037 = arith.andi %1012, %1029 : i1
    %1038 = arith.ori %1036, %1037 : i1
    %1039 = arith.andi %1035, %1038 : i1
    %1040 = arith.ori %1034, %1039 : i1
    %1041 = arith.andi %1013, %1040 : i1
    %1042 = arith.andi %997, %1041 : i1
    %1043 = arith.ori %1009, %1042 : i1
    %1044 = arith.andi %1043, %1043 : i1
    %1045 = acir.extract %0 {acir.id = "v85", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc56)
    %1046 = acir.extract %46 {acir.id = "v86", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc57)
    %1047 = arith.cmpi ult, %1045, %1046 {acir.id = "v87", acir.source_type = "bool"} : i32 loc(#loc57)
    %1048 = arith.select %1020, %1047, %true_2 : i1 loc(#loc57)
    %1049 = arith.select %998, %false_1, %1048 : i1 loc(#loc57)
    %1050 = arith.xori %1049, %true_0 : i1 loc(#loc57)
    %1051 = arith.xori %1050, %true_0 : i1 loc(#loc57)
    %1052 = arith.xori %1050, %true_0 : i1 loc(#loc58)
    %1053 = arith.xori %1050, %true_0 : i1 loc(#loc58)
    %1054 = arith.xori %1050, %true_0 : i1
    %1055 = arith.ori %1054, %1043 : i1
    %1056 = arith.ori %1053, %1055 : i1
    %1057 = arith.ori %1052, %1056 : i1
    %1058 = arith.andi %1057, %true : i1 loc(#loc12)
    %1059 = arith.ori %1051, %1058 : i1 loc(#loc12)
    %1060 = arith.andi %1044, %1059 : i1 loc(#loc12)
    %1061 = arith.xori %1050, %true_0 : i1
    %1062 = arith.andi %1057, %true : i1
    %1063 = arith.ori %1061, %1062 : i1
    %1064 = arith.andi %1060, %1063 : i1
    %1065 = arith.xori %1050, %true_0 : i1
    %1066 = arith.xori %1050, %true_0 : i1
    %1067 = arith.ori %1066, %1057 : i1
    %1068 = acir.extract %46 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc13)
    %1069 = arith.xori %1068, %true_0 : i1 loc(#loc13)
    %1070 = arith.andi %1050, %1069 : i1 loc(#loc13)
    %1071 = arith.xori %1070, %true_0 : i1
    %1072 = arith.andi %1057, %true : i1
    %1073 = arith.ori %1071, %1072 : i1
    %1074 = arith.andi %1070, %1073 : i1
    %1075 = arith.andi %1050, %1068 : i1
    %1076 = arith.xori %1075, %true_0 : i1
    %1077 = arith.andi %1057, %true : i1
    %1078 = arith.ori %1076, %1077 : i1
    %1079 = arith.andi %1050, %1055 : i1 loc(#loc58)
    %1080 = arith.andi %1056, %1079 : i1 loc(#loc58)
    %1081 = arith.andi %1050, %1080 : i1 loc(#loc58)
    %1082 = arith.andi %true, %1081 : i1 loc(#loc58)
    %1083 = arith.andi %1078, %1082 : i1 loc(#loc58)
    %1084 = arith.andi %1075, %1083 : i1 loc(#loc58)
    %1085 = arith.ori %1074, %1084 : i1 loc(#loc58)
    %1086 = arith.andi %1067, %1085 : i1 loc(#loc58)
    %1087 = arith.ori %1065, %1086 : i1 loc(#loc58)
    %1088 = arith.andi %1064, %1087 : i1 loc(#loc58)
    %1089 = arith.xori %1050, %true_0 : i1
    %1090 = arith.xori %1050, %true_0 : i1
    %1091 = arith.ori %1090, %1067 : i1
    %false_34 = arith.constant {acir.source_type = "bool"} false
    %1092 = acir.extract %46 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc14)
    %c1_i32_35 = arith.constant {acir.source_type = "u32"} 1 : i32
    %1093 = arith.cmpi eq, %1092, %c1_i32_35 {acir.id = "v7", acir.source_type = "bool"} : i32
    %1094 = arith.select %1070, %false_34, %1093 : i1
    %1095 = arith.xori %1094, %true_0 : i1
    %1096 = arith.andi %1050, %1095 : i1
    %1097 = arith.xori %1096, %true_0 : i1
    %1098 = arith.andi %1067, %1085 : i1
    %1099 = arith.ori %1097, %1098 : i1
    %1100 = arith.andi %1096, %1099 : i1
    %1101 = arith.andi %1050, %1094 : i1
    %1102 = arith.xori %1101, %true_0 : i1
    %1103 = arith.andi %1067, %1085 : i1
    %1104 = arith.ori %1102, %1103 : i1
    %1105 = arith.andi %1104, %true : i1
    %1106 = arith.andi %1101, %1105 : i1
    %1107 = arith.ori %1100, %1106 : i1
    %1108 = arith.andi %1091, %1107 : i1
    %1109 = arith.ori %1089, %1108 : i1
    %1110 = arith.andi %1088, %1109 : i1
    %1111 = arith.xori %1050, %true_0 : i1 loc(#loc15)
    %1112 = arith.xori %1050, %true_0 : i1
    %1113 = arith.ori %1112, %1091 : i1
    %1114 = acir.extract %46 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1115 = acir.extract %1114 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc16)
    %1116 = arith.select %1096, %false_34, %1115 : i1 loc(#loc16)
    %1117 = arith.xori %1116, %true_0 : i1 loc(#loc16)
    %1118 = arith.andi %1050, %1117 : i1 loc(#loc16)
    %1119 = arith.xori %1118, %true_0 : i1
    %1120 = arith.andi %1091, %1107 : i1
    %1121 = arith.ori %1119, %1120 : i1
    %1122 = arith.andi %1118, %1121 : i1
    %1123 = arith.andi %1050, %1116 : i1
    %1124 = arith.xori %1123, %true_0 : i1
    %1125 = arith.andi %1091, %1107 : i1
    %1126 = arith.ori %1124, %1125 : i1
    %1127 = arith.andi %1126, %true : i1
    %1128 = arith.andi %1123, %1127 : i1
    %1129 = arith.ori %1122, %1128 : i1
    %1130 = arith.andi %1113, %1129 : i1
    %1131 = arith.andi %1050, %1130 : i1
    %1132 = arith.andi %1056, %1131 : i1
    %1133 = arith.andi %1050, %1132 : i1
    %1134 = acir.extract %46 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc17)
    %1135 = acir.extract %1134 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc17)
    %1136 = arith.select %1118, %false_34, %1135 : i1 loc(#loc17)
    %1137 = arith.andi %1133, %1136 : i1 loc(#loc17)
    %1138 = arith.andi %1055, %1137 : i1 loc(#loc17)
    %1139 = arith.ori %1111, %1138 : i1 loc(#loc17)
    %1140 = arith.andi %1110, %1139 : i1 loc(#loc17)
    %1141 = arith.xori %1050, %true_0 : i1 loc(#loc18)
    %1142 = arith.xori %1050, %true_0 : i1 loc(#loc15)
    %1143 = arith.andi %1055, %1137 : i1 loc(#loc15)
    %1144 = arith.ori %1142, %1143 : i1 loc(#loc15)
    %1145 = arith.andi %1144, %true : i1 loc(#loc18)
    %1146 = arith.ori %1141, %1145 : i1 loc(#loc18)
    %1147 = arith.andi %1140, %1146 : i1 loc(#loc18)
    %1148 = arith.xori %1050, %true_0 : i1 loc(#loc19)
    %1149 = arith.ori %1148, %1144 : i1 loc(#loc19)
    %1150 = arith.andi %1147, %1149 : i1 loc(#loc19)
    %1151 = arith.xori %1049, %true_0 : i1 loc(#loc12)
    %1152 = arith.xori %1049, %true_0 : i1 loc(#loc59)
    %1153 = arith.xori %1049, %true_0 : i1 loc(#loc59)
    %1154 = arith.xori %1049, %true_0 : i1 loc(#loc60)
    %1155 = arith.ori %1154, %1043 : i1 loc(#loc60)
    %1156 = arith.ori %1153, %1155 : i1 loc(#loc60)
    %1157 = arith.ori %1152, %1156 : i1 loc(#loc60)
    %1158 = arith.andi %1157, %true : i1 loc(#loc12)
    %1159 = arith.ori %1151, %1158 : i1 loc(#loc12)
    %1160 = arith.andi %1150, %1159 : i1 loc(#loc12)
    %1161 = arith.xori %1049, %true_0 : i1
    %1162 = arith.andi %1157, %true : i1
    %1163 = arith.ori %1161, %1162 : i1
    %1164 = arith.andi %1160, %1163 : i1
    %1165 = arith.xori %1049, %true_0 : i1
    %1166 = arith.xori %1049, %true_0 : i1
    %1167 = arith.ori %1166, %1157 : i1
    %1168 = acir.extract %0 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc13)
    %1169 = arith.xori %1168, %true_0 : i1 loc(#loc13)
    %1170 = arith.andi %1049, %1169 : i1 loc(#loc13)
    %1171 = arith.xori %1170, %true_0 : i1
    %1172 = arith.andi %1157, %true : i1
    %1173 = arith.ori %1171, %1172 : i1
    %1174 = arith.andi %1170, %1173 : i1
    %1175 = arith.andi %1049, %1168 : i1
    %1176 = arith.xori %1175, %true_0 : i1
    %1177 = arith.andi %1157, %true : i1
    %1178 = arith.ori %1176, %1177 : i1
    %1179 = arith.andi %1049, %1155 : i1 loc(#loc59)
    %1180 = arith.andi %1156, %1179 : i1 loc(#loc59)
    %1181 = arith.andi %1049, %1180 : i1 loc(#loc59)
    %1182 = arith.andi %true, %1181 : i1 loc(#loc59)
    %1183 = arith.andi %1178, %1182 : i1 loc(#loc59)
    %1184 = arith.andi %1175, %1183 : i1 loc(#loc59)
    %1185 = arith.ori %1174, %1184 : i1 loc(#loc59)
    %1186 = arith.andi %1167, %1185 : i1 loc(#loc59)
    %1187 = arith.ori %1165, %1186 : i1 loc(#loc59)
    %1188 = arith.andi %1164, %1187 : i1 loc(#loc59)
    %1189 = arith.xori %1049, %true_0 : i1
    %1190 = arith.xori %1049, %true_0 : i1
    %1191 = arith.ori %1190, %1167 : i1
    %false_36 = arith.constant {acir.source_type = "bool"} false
    %1192 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc14)
    %1193 = arith.cmpi eq, %1192, %c1_i32_35 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc14)
    %1194 = arith.select %1170, %false_36, %1193 : i1 loc(#loc14)
    %1195 = arith.xori %1194, %true_0 : i1 loc(#loc14)
    %1196 = arith.andi %1049, %1195 : i1 loc(#loc14)
    %1197 = arith.xori %1196, %true_0 : i1
    %1198 = arith.andi %1167, %1185 : i1
    %1199 = arith.ori %1197, %1198 : i1
    %1200 = arith.andi %1196, %1199 : i1
    %1201 = arith.andi %1049, %1194 : i1
    %1202 = arith.xori %1201, %true_0 : i1
    %1203 = arith.andi %1167, %1185 : i1
    %1204 = arith.ori %1202, %1203 : i1
    %1205 = arith.andi %1204, %true : i1
    %1206 = arith.andi %1201, %1205 : i1
    %1207 = arith.ori %1200, %1206 : i1
    %1208 = arith.andi %1191, %1207 : i1
    %1209 = arith.ori %1189, %1208 : i1
    %1210 = arith.andi %1188, %1209 : i1
    %1211 = arith.xori %1049, %true_0 : i1 loc(#loc15)
    %1212 = arith.xori %1049, %true_0 : i1
    %1213 = arith.ori %1212, %1191 : i1
    %1214 = acir.extract %0 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1215 = acir.extract %1214 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc16)
    %1216 = arith.select %1196, %false_36, %1215 : i1 loc(#loc16)
    %1217 = arith.xori %1216, %true_0 : i1 loc(#loc16)
    %1218 = arith.andi %1049, %1217 : i1 loc(#loc16)
    %1219 = arith.xori %1218, %true_0 : i1
    %1220 = arith.andi %1191, %1207 : i1
    %1221 = arith.ori %1219, %1220 : i1
    %1222 = arith.andi %1218, %1221 : i1
    %1223 = arith.andi %1049, %1216 : i1
    %1224 = arith.xori %1223, %true_0 : i1
    %1225 = arith.andi %1191, %1207 : i1
    %1226 = arith.ori %1224, %1225 : i1
    %1227 = arith.andi %1226, %true : i1
    %1228 = arith.andi %1223, %1227 : i1
    %1229 = arith.ori %1222, %1228 : i1
    %1230 = arith.andi %1213, %1229 : i1
    %1231 = arith.andi %1049, %1230 : i1
    %1232 = arith.andi %1156, %1231 : i1
    %1233 = arith.andi %1049, %1232 : i1
    %1234 = acir.extract %0 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc17)
    %1235 = acir.extract %1234 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc17)
    %1236 = arith.select %1218, %false_36, %1235 : i1 loc(#loc17)
    %1237 = arith.andi %1233, %1236 : i1 loc(#loc17)
    %1238 = arith.andi %1155, %1237 : i1 loc(#loc17)
    %1239 = arith.ori %1211, %1238 : i1 loc(#loc17)
    %1240 = arith.andi %1210, %1239 : i1 loc(#loc17)
    %1241 = arith.xori %1049, %true_0 : i1 loc(#loc18)
    %1242 = arith.xori %1049, %true_0 : i1 loc(#loc15)
    %1243 = arith.andi %1155, %1237 : i1 loc(#loc15)
    %1244 = arith.ori %1242, %1243 : i1 loc(#loc15)
    %1245 = arith.andi %1244, %true : i1 loc(#loc18)
    %1246 = arith.ori %1241, %1245 : i1 loc(#loc18)
    %1247 = arith.andi %1240, %1246 : i1 loc(#loc18)
    %1248 = arith.xori %1049, %true_0 : i1 loc(#loc19)
    %1249 = arith.ori %1248, %1244 : i1 loc(#loc19)
    %1250 = arith.andi %1247, %1249 : i1 loc(#loc19)
    %1251 = arith.andi %1144, %true : i1 loc(#loc22)
    %1252 = arith.andi %1050, %1251 : i1 loc(#loc22)
    %1253 = arith.andi %1252, %1137 : i1 loc(#loc22)
    %1254 = arith.andi %1055, %1253 : i1 loc(#loc22)
    %1255 = arith.andi %1050, %1254 : i1 loc(#loc22)
    %1256 = arith.andi %1244, %true : i1 loc(#loc22)
    %1257 = arith.andi %1049, %1256 : i1 loc(#loc22)
    %1258 = arith.andi %1257, %1237 : i1 loc(#loc22)
    %1259 = arith.andi %1155, %1258 : i1 loc(#loc22)
    %1260 = arith.andi %1049, %1259 : i1 loc(#loc22)
    %1261 = arith.ori %1255, %1260 : i1 loc(#loc22)
    %1262 = arith.xori %1261, %true_0 : i1 loc(#loc22)
    %1263 = acir.current {aspect = "count", resource = "top.scheduler.mul_requests"} : i32 loc(#loc61)
    %1264 = acir.current {aspect = "value", resource = "top.multiplier.active"} : i1 loc(#loc62)
    %1265 = arith.xori %1264, %true_0 : i1 loc(#loc62)
    %1266 = arith.xori %1265, %true_0 : i1
    %1267 = arith.ori %1266, %true : i1
    %1268 = arith.andi %1265, %1267 : i1
    %1269 = arith.xori %1265, %true_0 : i1 loc(#loc63)
    %1270 = arith.xori %1265, %true_0 : i1 loc(#loc64)
    %1271 = arith.ori %1270, %1267 : i1 loc(#loc64)
    %1272 = acir.current {aspect = "storage", resource = "top.scheduler.mul_requests"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc63)
    %c0_i32_37 = arith.constant 0 : i32 loc(#loc63)
    %1273 = acir.extract %1272[%c0_i32_37] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc63)
    %c0_i32_38 = arith.constant 0 : i32 loc(#loc63)
    %1274 = arith.cmpi ne, %1263, %c0_i32_38 : i32 loc(#loc63)
    %1275 = arith.andi %1271, %1274 : i1 loc(#loc63)
    %1276 = arith.ori %1269, %1275 : i1 loc(#loc63)
    %1277 = arith.andi %1268, %1276 : i1 loc(#loc63)
    %1278 = arith.xori %1265, %true_0 : i1 loc(#loc63)
    %1279 = arith.andi %1271, %1274 : i1 loc(#loc63)
    %1280 = arith.ori %1278, %1279 : i1 loc(#loc63)
    %1281 = arith.andi %1277, %1280 : i1 loc(#loc63)
    %1282 = arith.xori %1265, %true_0 : i1 loc(#loc63)
    %c0_i32_39 = arith.constant 0 : i32 loc(#loc63)
    %1283 = arith.cmpi eq, %1263, %c0_i32_39 : i32 loc(#loc63)
    %1284 = arith.andi %1271, %true : i1 loc(#loc63)
    %1285 = arith.ori %1282, %1284 : i1 loc(#loc63)
    %1286 = arith.andi %1281, %1285 : i1 loc(#loc63)
    %1287 = arith.xori %1265, %true_0 : i1 loc(#loc63)
    %1288 = arith.xori %1283, %true_0 : i1 loc(#loc63)
    %1289 = arith.andi %1271, %1288 : i1 loc(#loc63)
    %1290 = arith.ori %1287, %1289 : i1 loc(#loc63)
    %1291 = arith.andi %1286, %1290 : i1 loc(#loc63)
    %1292 = arith.xori %1265, %true_0 : i1 loc(#loc65)
    %1293 = arith.andi %1271, %1274 : i1 loc(#loc65)
    %1294 = arith.ori %1292, %1293 : i1 loc(#loc65)
    %1295 = arith.andi %1291, %1294 : i1 loc(#loc65)
    %1296 = arith.xori %1265, %true_0 : i1 loc(#loc66)
    %1297 = arith.ori %1296, %1271 : i1 loc(#loc66)
    %1298 = arith.andi %1295, %1297 : i1 loc(#loc66)
    %1299 = arith.xori %1265, %true_0 : i1 loc(#loc67)
    %1300 = arith.ori %1299, %1271 : i1 loc(#loc67)
    %1301 = arith.andi %1298, %1300 : i1 loc(#loc67)
    %1302 = acir.fire "top.multiplier.accept#3" %1301 {successors = [1], txn = 3 : i64} loc(#loc67)
    %1303 = arith.andi %1302, %1265 : i1 loc(#loc67)
    %c0_i32_40 = arith.constant 0 : i32 loc(#loc67)
    %c1_i32_41 = arith.constant 1 : i32 loc(#loc67)
    %c0_i32_42 = arith.constant 0 : i32 loc(#loc67)
    %1304 = arith.select %1303, %c1_i32_41, %c0_i32_42 : i32 loc(#loc67)
    %1305 = arith.addi %c0_i32_40, %1304 : i32 loc(#loc67)
    %1306 = arith.subi %1263, %1305 : i32 loc(#loc61)
    %c1_i32_43 = arith.constant 1 : i32 loc(#loc67)
    %1307 = arith.cmpi uge, %1306, %c1_i32_43 : i32 loc(#loc67)
    %1308 = arith.ori %1262, %true : i1 loc(#loc67)
    %1309 = arith.andi %1250, %1308 : i1 loc(#loc67)
    %1310 = arith.xori %1261, %true_0 : i1 loc(#loc61)
    %1311 = arith.xori %1307, %true_0 : i1 loc(#loc61)
    %1312 = arith.ori %1310, %1311 : i1 loc(#loc61)
    %1313 = arith.andi %1309, %1312 : i1 loc(#loc61)
    %1314 = acir.fire "top.scheduler.issue_mul#1" %1313 {successors = [], txn = 1 : i64} loc(#loc61)
    %1315 = arith.xori %1264, %true_0 : i1
    %1316 = arith.ori %1315, %true : i1
    %1317 = arith.andi %1264, %1316 : i1
    %1318 = arith.xori %1264, %true_0 : i1 loc(#loc68)
    %1319 = arith.xori %1264, %true_0 : i1 loc(#loc69)
    %1320 = arith.ori %1319, %1316 : i1 loc(#loc69)
    %1321 = acir.current {aspect = "value", resource = "top.multiplier.remaining"} : i32 loc(#loc68)
    %1322 = arith.andi %1320, %true : i1 loc(#loc68)
    %1323 = arith.ori %1318, %1322 : i1 loc(#loc68)
    %1324 = arith.andi %1317, %1323 : i1 loc(#loc68)
    %1325 = arith.xori %1264, %true_0 : i1
    %1326 = arith.andi %1320, %true : i1
    %1327 = arith.ori %1325, %1326 : i1
    %1328 = arith.andi %1324, %1327 : i1
    %c0_i32_44 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc70)
    %1329 = arith.cmpi ne, %1321, %c0_i32_44 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc70)
    %1330 = arith.xori %1329, %true_0 : i1 loc(#loc70)
    %1331 = arith.andi %1264, %1330 : i1 loc(#loc70)
    %1332 = arith.xori %1331, %true_0 : i1 loc(#loc70)
    %1333 = arith.xori %1331, %true_0 : i1
    %1334 = arith.andi %1320, %true : i1
    %1335 = arith.ori %1333, %1334 : i1
    %1336 = acir.current {aspect = "value", resource = "top.multiplier.request"} : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc71)
    %1337 = arith.andi %1335, %true : i1 loc(#loc71)
    %1338 = arith.ori %1332, %1337 : i1 loc(#loc71)
    %1339 = arith.andi %1328, %1338 : i1 loc(#loc71)
    %1340 = arith.xori %1331, %true_0 : i1 loc(#loc72)
    %1341 = arith.ori %1340, %1335 : i1 loc(#loc72)
    %1342 = arith.andi %1339, %1341 : i1 loc(#loc72)
    %1343 = arith.andi %1264, %1329 : i1
    %1344 = arith.xori %1343, %true_0 : i1
    %1345 = arith.xori %1343, %true_0 : i1 loc(#loc73)
    %1346 = arith.andi %1320, %true : i1
    %1347 = arith.ori %1345, %1346 : i1
    %1348 = arith.andi %1347, %true : i1 loc(#loc74)
    %1349 = arith.ori %1344, %1348 : i1 loc(#loc74)
    %1350 = arith.andi %1342, %1349 : i1 loc(#loc74)
    %1351 = arith.xori %1343, %true_0 : i1 loc(#loc75)
    %1352 = arith.andi %1347, %true : i1 loc(#loc75)
    %1353 = arith.ori %1351, %1352 : i1 loc(#loc75)
    %1354 = arith.andi %1350, %1353 : i1 loc(#loc75)
    %1355 = arith.andi %true, %true : i1 loc(#loc76)
    %1356 = arith.andi %true, %true : i1 loc(#loc77)
    %1357 = arith.andi %1355, %1356 : i1 loc(#loc77)
    %1358 = arith.andi %1335, %1357 : i1 loc(#loc77)
    %1359 = arith.andi %1331, %1358 : i1 loc(#loc77)
    %1360 = arith.andi %1264, %1359 : i1 loc(#loc77)
    %1361 = arith.xori %1360, %true_0 : i1 loc(#loc77)
    %1362 = arith.ori %1361, %1316 : i1 loc(#loc77)
    %1363 = arith.andi %1354, %1362 : i1 loc(#loc77)
    %1364 = arith.xori %1360, %true_0 : i1 loc(#loc78)
    %1365 = arith.andi %970, %338 : i1 loc(#loc79)
    %c0_i32_45 = arith.constant 0 : i32 loc(#loc79)
    %c1_i32_46 = arith.constant 1 : i32 loc(#loc79)
    %c0_i32_47 = arith.constant 0 : i32 loc(#loc79)
    %1366 = arith.select %1365, %c1_i32_46, %c0_i32_47 : i32 loc(#loc79)
    %1367 = arith.addi %c0_i32_45, %1366 : i32 loc(#loc79)
    %1368 = arith.subi %344, %1367 : i32 loc(#loc78)
    %c1_i32_48 = arith.constant 1 : i32 loc(#loc79)
    %1369 = arith.cmpi uge, %1368, %c1_i32_48 : i32 loc(#loc79)
    %1370 = arith.andi %1316, %true : i1 loc(#loc79)
    %1371 = arith.ori %1364, %1370 : i1 loc(#loc79)
    %1372 = arith.andi %1363, %1371 : i1 loc(#loc79)
    %1373 = arith.xori %1360, %true_0 : i1 loc(#loc78)
    %1374 = arith.xori %1369, %true_0 : i1 loc(#loc78)
    %1375 = arith.andi %1316, %1374 : i1 loc(#loc78)
    %1376 = arith.ori %1373, %1375 : i1 loc(#loc78)
    %1377 = arith.andi %1372, %1376 : i1 loc(#loc78)
    %1378 = acir.fire "top.multiplier.advance#4" %1377 {successors = [], txn = 4 : i64} loc(#loc78)
    %c0_i32_49 = arith.constant 0 : i32 loc(#loc80)
    %1379 = arith.cmpi eq, %334, %c0_i32_49 : i32 loc(#loc80)
    %1380 = arith.xori %1379, %true_0 : i1 loc(#loc80)
    %false_50 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_51 = arith.constant 0 : i32 loc(#loc81)
    %1381 = arith.cmpi eq, %344, %c0_i32_51 : i32 loc(#loc81)
    %1382 = arith.select %1380, %false_50, %1381 : i1 loc(#loc81)
    %1383 = arith.xori %1382, %true_0 : i1
    %1384 = arith.xori %1380, %true_0 : i1
    %1385 = arith.ori %1384, %true : i1
    %1386 = arith.andi %1380, %1385 : i1
    %1387 = arith.xori %1379, %true_0 : i1
    %1388 = arith.ori %1387, %true : i1
    %1389 = arith.andi %1388, %true : i1
    %1390 = arith.andi %1379, %1389 : i1
    %1391 = arith.ori %1386, %1390 : i1
    %1392 = arith.ori %1383, %1391 : i1
    %1393 = arith.andi %1382, %1392 : i1
    %1394 = arith.xori %1382, %true_0 : i1 loc(#loc82)
    %1395 = arith.xori %1382, %true_0 : i1 loc(#loc83)
    %1396 = arith.ori %1395, %1392 : i1 loc(#loc83)
    %1397 = acir.current {aspect = "storage", resource = "top.source"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4> loc(#loc82)
    %c0_i32_52 = arith.constant 0 : i32 loc(#loc82)
    %1398 = acir.extract %1397[%c0_i32_52] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc82)
    %1399 = acir.current {aspect = "count", resource = "top.source"} : i32 loc(#loc82)
    %c0_i32_53 = arith.constant 0 : i32 loc(#loc82)
    %1400 = arith.cmpi ne, %1399, %c0_i32_53 : i32 loc(#loc82)
    %1401 = arith.andi %1396, %1400 : i1 loc(#loc82)
    %1402 = arith.ori %1394, %1401 : i1 loc(#loc82)
    %1403 = arith.andi %1393, %1402 : i1 loc(#loc82)
    %1404 = arith.xori %1382, %true_0 : i1 loc(#loc82)
    %1405 = arith.andi %1396, %1400 : i1 loc(#loc82)
    %1406 = arith.ori %1404, %1405 : i1 loc(#loc82)
    %1407 = arith.andi %1403, %1406 : i1 loc(#loc82)
    %1408 = arith.xori %1382, %true_0 : i1 loc(#loc82)
    %c0_i32_54 = arith.constant 0 : i32 loc(#loc82)
    %1409 = arith.cmpi eq, %1399, %c0_i32_54 : i32 loc(#loc82)
    %1410 = arith.andi %1396, %true : i1 loc(#loc82)
    %1411 = arith.ori %1408, %1410 : i1 loc(#loc82)
    %1412 = arith.andi %1407, %1411 : i1 loc(#loc82)
    %1413 = arith.xori %1382, %true_0 : i1 loc(#loc82)
    %1414 = arith.xori %1409, %true_0 : i1 loc(#loc82)
    %1415 = arith.andi %1396, %1414 : i1 loc(#loc82)
    %1416 = arith.ori %1413, %1415 : i1 loc(#loc82)
    %1417 = arith.andi %1412, %1416 : i1 loc(#loc82)
    %1418 = arith.xori %1382, %true_0 : i1 loc(#loc84)
    %1419 = arith.xori %1382, %true_0 : i1 loc(#loc85)
    %1420 = arith.ori %1419, %1396 : i1 loc(#loc85)
    %1421 = arith.andi %1396, %1400 : i1 loc(#loc85)
    %1422 = arith.andi %1382, %1421 : i1 loc(#loc85)
    %1423 = arith.andi %1420, %1422 : i1 loc(#loc85)
    %1424 = arith.ori %1418, %1423 : i1 loc(#loc85)
    %1425 = arith.andi %1417, %1424 : i1 loc(#loc85)
    %1426 = arith.xori %1382, %true_0 : i1
    %1427 = arith.andi %1420, %1422 : i1
    %1428 = arith.ori %1426, %1427 : i1
    %1429 = arith.andi %1425, %1428 : i1
    %1430 = acir.extract %1398 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc86)
    %c0_i32_55 = arith.constant {acir.source_type = "u32"} 0 : i32
    %1431 = arith.cmpi eq, %1430, %c0_i32_55 {acir.id = "v8", acir.source_type = "bool"} : i32
    %1432 = arith.xori %1431, %true_0 : i1
    %1433 = arith.andi %1382, %1432 : i1
    %1434 = arith.xori %1433, %true_0 : i1
    %1435 = arith.xori %1433, %true_0 : i1
    %1436 = arith.andi %1420, %1422 : i1
    %1437 = arith.ori %1435, %1436 : i1
    %1438 = acir.current {aspect = "value", resource = "top.rf.renamed"} : !acir.array<i32, 8> loc(#loc87)
    %1439 = arith.andi %1437, %true : i1 loc(#loc87)
    %1440 = arith.ori %1434, %1439 : i1 loc(#loc87)
    %1441 = arith.andi %1429, %1440 : i1 loc(#loc87)
    %1442 = arith.xori %1433, %true_0 : i1 loc(#loc88)
    %1443 = acir.current {aspect = "value", resource = "top.rf.committed"} : !acir.array<i32, 8> loc(#loc88)
    %1444 = arith.andi %1437, %true : i1 loc(#loc88)
    %1445 = arith.ori %1442, %1444 : i1 loc(#loc88)
    %1446 = arith.andi %1441, %1445 : i1 loc(#loc88)
    %1447 = arith.xori %1433, %true_0 : i1
    %1448 = arith.andi %1436, %1422 : i1
    %1449 = arith.andi %1433, %1448 : i1
    %1450 = arith.andi %true, %1449 : i1
    %1451 = arith.andi %true, %1449 : i1 loc(#loc88)
    %1452 = arith.andi %1450, %1451 : i1 loc(#loc88)
    %1453 = arith.andi %1437, %1452 : i1 loc(#loc88)
    %1454 = arith.ori %1447, %1453 : i1 loc(#loc88)
    %1455 = arith.andi %1446, %1454 : i1 loc(#loc88)
    %1456 = acir.extract %1438[%1430] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc87)
    %1457 = acir.extract %1443[%1430] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc88)
    %1458 = arith.cmpi eq, %1456, %1457 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc88)
    %1459 = arith.xori %1458, %true_0 : i1 loc(#loc88)
    %1460 = arith.andi %1433, %1459 : i1 loc(#loc88)
    %1461 = arith.xori %1460, %true_0 : i1 loc(#loc88)
    %1462 = arith.xori %1460, %true_0 : i1 loc(#loc89)
    %1463 = arith.xori %1460, %true_0 : i1
    %1464 = arith.andi %1437, %1452 : i1
    %1465 = arith.ori %1463, %1464 : i1
    %1466 = arith.ori %1462, %1465 : i1
    %1467 = arith.andi %1464, %1450 : i1
    %1468 = arith.andi %1460, %1467 : i1
    %1469 = arith.andi %1465, %1468 : i1
    %1470 = arith.andi %1460, %1469 : i1
    %1471 = arith.andi %1466, %1470 : i1
    %1472 = arith.ori %1461, %1471 : i1
    %1473 = arith.andi %1455, %1472 : i1
    %c1_i32_56 = arith.constant {acir.source_type = "u32"} 1 : i32
    %1474 = arith.subi %1456, %c1_i32_56 {acir.id = "v5", acir.source_type = "u32"} : i32
    %c3_i32_57 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc90)
    %1475 = arith.andi %1474, %c3_i32_57 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc90)
    %c0_i32_58 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc91)
    %1476 = arith.cmpi eq, %1475, %c0_i32_58 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc91)
    %1477 = arith.xori %1476, %true_0 : i1 loc(#loc91)
    %1478 = arith.andi %1460, %1477 : i1 loc(#loc91)
    %1479 = arith.xori %1478, %true_0 : i1 loc(#loc91)
    %1480 = arith.xori %1478, %true_0 : i1
    %1481 = arith.andi %1466, %1470 : i1
    %1482 = arith.ori %1480, %1481 : i1
    %1483 = arith.andi %1482, %1470 : i1
    %1484 = arith.ori %1479, %1483 : i1
    %1485 = arith.andi %1473, %1484 : i1
    %1486 = arith.cmpi eq, %1475, %c1_i32_56 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc92)
    %1487 = arith.xori %1486, %true_0 : i1 loc(#loc92)
    %1488 = arith.andi %1478, %1487 : i1 loc(#loc92)
    %1489 = arith.xori %1488, %true_0 : i1 loc(#loc92)
    %1490 = arith.xori %1488, %true_0 : i1
    %1491 = arith.andi %1482, %1470 : i1
    %1492 = arith.ori %1490, %1491 : i1
    %1493 = arith.andi %1492, %1470 : i1
    %1494 = arith.ori %1489, %1493 : i1
    %1495 = arith.andi %1485, %1494 : i1
    %c2_i32_59 = arith.constant {acir.source_type = "u32"} 2 : i32
    %1496 = arith.cmpi eq, %1475, %c2_i32_59 {acir.id = "v19", acir.source_type = "bool"} : i32
    %1497 = arith.xori %1496, %true_0 : i1
    %1498 = arith.andi %1488, %1497 : i1
    %1499 = arith.xori %1498, %true_0 : i1
    %1500 = arith.xori %1498, %true_0 : i1 loc(#loc93)
    %1501 = arith.xori %1498, %true_0 : i1
    %1502 = arith.andi %1492, %1470 : i1
    %1503 = arith.ori %1501, %1502 : i1
    %1504 = arith.ori %1500, %1503 : i1
    %1505 = arith.andi %1504, %true : i1 loc(#loc94)
    %1506 = arith.ori %1499, %1505 : i1 loc(#loc94)
    %1507 = arith.andi %1495, %1506 : i1 loc(#loc94)
    %1508 = arith.andi %1488, %1496 : i1
    %1509 = arith.xori %1508, %true_0 : i1
    %1510 = arith.xori %1508, %true_0 : i1 loc(#loc95)
    %1511 = arith.xori %1508, %true_0 : i1 loc(#loc96)
    %1512 = arith.andi %1492, %1470 : i1
    %1513 = arith.ori %1511, %1512 : i1
    %1514 = arith.ori %1510, %1513 : i1
    %1515 = arith.andi %1514, %true : i1 loc(#loc94)
    %1516 = arith.ori %1509, %1515 : i1 loc(#loc94)
    %1517 = arith.andi %1507, %1516 : i1 loc(#loc94)
    %1518 = arith.andi %1478, %1486 : i1
    %1519 = arith.xori %1518, %true_0 : i1
    %1520 = arith.xori %1518, %true_0 : i1 loc(#loc97)
    %1521 = arith.xori %1518, %true_0 : i1 loc(#loc98)
    %1522 = arith.andi %1482, %1470 : i1
    %1523 = arith.ori %1521, %1522 : i1
    %1524 = arith.ori %1520, %1523 : i1
    %1525 = arith.andi %1524, %true : i1 loc(#loc94)
    %1526 = arith.ori %1519, %1525 : i1 loc(#loc94)
    %1527 = arith.andi %1517, %1526 : i1 loc(#loc94)
    %1528 = arith.andi %1460, %1476 : i1
    %1529 = arith.xori %1528, %true_0 : i1
    %1530 = arith.xori %1528, %true_0 : i1 loc(#loc99)
    %1531 = arith.xori %1528, %true_0 : i1 loc(#loc100)
    %1532 = arith.andi %1466, %1470 : i1
    %1533 = arith.ori %1531, %1532 : i1
    %1534 = arith.ori %1530, %1533 : i1
    %1535 = arith.andi %1534, %true : i1 loc(#loc94)
    %1536 = arith.ori %1529, %1535 : i1 loc(#loc94)
    %1537 = arith.andi %1527, %1536 : i1 loc(#loc94)
    %1538 = arith.xori %1460, %true_0 : i1
    %1539 = arith.andi %1504, %true : i1 loc(#loc101)
    %1540 = arith.andi %1498, %1539 : i1 loc(#loc101)
    %1541 = arith.andi %1503, %1540 : i1 loc(#loc101)
    %1542 = arith.andi %1498, %1541 : i1 loc(#loc101)
    %1543 = arith.andi %1514, %true : i1 loc(#loc101)
    %1544 = arith.andi %1508, %1543 : i1 loc(#loc101)
    %1545 = arith.andi %1513, %1544 : i1 loc(#loc101)
    %1546 = arith.andi %1508, %1545 : i1 loc(#loc101)
    %1547 = arith.ori %1542, %1546 : i1 loc(#loc101)
    %1548 = arith.andi %1524, %true : i1 loc(#loc101)
    %1549 = arith.andi %1518, %1548 : i1 loc(#loc101)
    %1550 = arith.andi %1523, %1549 : i1 loc(#loc101)
    %1551 = arith.andi %1518, %1550 : i1 loc(#loc101)
    %1552 = arith.ori %1547, %1551 : i1 loc(#loc101)
    %1553 = arith.andi %1534, %true : i1 loc(#loc101)
    %1554 = arith.andi %1528, %1553 : i1 loc(#loc101)
    %1555 = arith.andi %1533, %1554 : i1 loc(#loc101)
    %1556 = arith.andi %1528, %1555 : i1 loc(#loc101)
    %1557 = arith.ori %1552, %1556 : i1 loc(#loc101)
    %1558 = arith.andi %1465, %1557 : i1 loc(#loc101)
    %1559 = arith.ori %1538, %1558 : i1 loc(#loc101)
    %1560 = arith.andi %1537, %1559 : i1 loc(#loc101)
    %1561 = arith.xori %1460, %true_0 : i1
    %1562 = arith.select %1518, %571, %643 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc102)
    %1563 = arith.select %1508, %497, %1562 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc102)
    %1564 = arith.select %1498, %423, %1563 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc102)
    %1565 = acir.extract %1564 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc102)
    %1566 = arith.xori %1565, %true_0 : i1 loc(#loc102)
    %1567 = arith.andi %1460, %1566 : i1 loc(#loc102)
    %1568 = arith.andi %1465, %1557 : i1
    %1569 = arith.andi %1567, %1568 : i1
    %1570 = arith.andi %1460, %1565 : i1
    %1571 = arith.xori %1570, %true_0 : i1
    %1572 = arith.andi %1465, %1557 : i1
    %1573 = arith.ori %1571, %1572 : i1
    %1574 = arith.andi %1573, %1557 : i1
    %1575 = arith.andi %1570, %1574 : i1
    %1576 = arith.ori %1569, %1575 : i1
    %1577 = arith.andi %1465, %1576 : i1
    %1578 = arith.ori %1561, %1577 : i1
    %1579 = arith.andi %1560, %1578 : i1
    %1580 = arith.xori %1460, %true_0 : i1
    %false_60 = arith.constant {acir.source_type = "bool"} false
    %1581 = acir.extract %1564 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc103)
    %1582 = arith.select %1567, %false_60, %1581 : i1 loc(#loc103)
    %1583 = arith.xori %1582, %true_0 : i1 loc(#loc103)
    %1584 = arith.andi %1460, %1583 : i1 loc(#loc103)
    %1585 = arith.andi %1465, %1576 : i1
    %1586 = arith.andi %1584, %1585 : i1
    %1587 = arith.andi %1460, %1582 : i1
    %1588 = arith.xori %1587, %true_0 : i1
    %1589 = arith.andi %1465, %1576 : i1
    %1590 = arith.ori %1588, %1589 : i1
    %1591 = arith.andi %1557, %1468 : i1 loc(#loc104)
    %1592 = arith.andi %1590, %1591 : i1 loc(#loc104)
    %1593 = arith.andi %1587, %1592 : i1 loc(#loc104)
    %1594 = arith.ori %1586, %1593 : i1 loc(#loc104)
    %1595 = arith.andi %1465, %1594 : i1 loc(#loc104)
    %1596 = arith.ori %1580, %1595 : i1 loc(#loc104)
    %1597 = arith.andi %1579, %1596 : i1 loc(#loc104)
    %1598 = arith.andi %1433, %1458 : i1
    %1599 = arith.xori %1598, %true_0 : i1
    %1600 = arith.xori %1598, %true_0 : i1 loc(#loc105)
    %1601 = arith.andi %1437, %1452 : i1
    %1602 = arith.ori %1600, %1601 : i1
    %1603 = acir.current {aspect = "value", resource = "top.rf.registers"} : !acir.array<i32, 8> loc(#loc106)
    %1604 = arith.andi %1602, %true : i1 loc(#loc106)
    %1605 = arith.ori %1599, %1604 : i1 loc(#loc106)
    %1606 = arith.andi %1597, %1605 : i1 loc(#loc106)
    %1607 = arith.xori %1382, %true_0 : i1 loc(#loc84)
    %1608 = arith.xori %1382, %true_0 : i1 loc(#loc107)
    %1609 = arith.ori %1608, %1396 : i1 loc(#loc107)
    %1610 = arith.andi %1396, %1400 : i1 loc(#loc107)
    %1611 = arith.andi %1382, %1610 : i1 loc(#loc107)
    %1612 = arith.andi %1609, %1611 : i1 loc(#loc107)
    %1613 = arith.ori %1607, %1612 : i1 loc(#loc107)
    %1614 = arith.andi %1606, %1613 : i1 loc(#loc107)
    %1615 = arith.xori %1382, %true_0 : i1
    %1616 = arith.andi %1609, %1611 : i1
    %1617 = arith.ori %1615, %1616 : i1
    %1618 = arith.andi %1614, %1617 : i1
    %1619 = acir.extract %1398 {acir.id = "v7", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc108)
    %c0_i32_61 = arith.constant {acir.source_type = "u32"} 0 : i32
    %1620 = arith.cmpi eq, %1619, %c0_i32_61 {acir.id = "v8", acir.source_type = "bool"} : i32
    %1621 = arith.xori %1620, %true_0 : i1
    %1622 = arith.andi %1382, %1621 : i1
    %1623 = arith.xori %1622, %true_0 : i1
    %1624 = arith.xori %1622, %true_0 : i1
    %1625 = arith.andi %1609, %1611 : i1
    %1626 = arith.ori %1624, %1625 : i1
    %1627 = arith.andi %1626, %true : i1 loc(#loc87)
    %1628 = arith.ori %1623, %1627 : i1 loc(#loc87)
    %1629 = arith.andi %1618, %1628 : i1 loc(#loc87)
    %1630 = arith.xori %1622, %true_0 : i1 loc(#loc88)
    %1631 = arith.andi %1626, %true : i1 loc(#loc88)
    %1632 = arith.ori %1630, %1631 : i1 loc(#loc88)
    %1633 = arith.andi %1629, %1632 : i1 loc(#loc88)
    %1634 = arith.xori %1622, %true_0 : i1
    %1635 = arith.andi %1625, %1611 : i1
    %1636 = arith.andi %1622, %1635 : i1
    %1637 = arith.andi %true, %1636 : i1
    %1638 = arith.andi %true, %1636 : i1 loc(#loc88)
    %1639 = arith.andi %1637, %1638 : i1 loc(#loc88)
    %1640 = arith.andi %1626, %1639 : i1 loc(#loc88)
    %1641 = arith.ori %1634, %1640 : i1 loc(#loc88)
    %1642 = arith.andi %1633, %1641 : i1 loc(#loc88)
    %1643 = acir.extract %1438[%1619] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc87)
    %1644 = acir.extract %1443[%1619] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc88)
    %1645 = arith.cmpi eq, %1643, %1644 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc88)
    %1646 = arith.xori %1645, %true_0 : i1 loc(#loc88)
    %1647 = arith.andi %1622, %1646 : i1 loc(#loc88)
    %1648 = arith.xori %1647, %true_0 : i1 loc(#loc88)
    %1649 = arith.xori %1647, %true_0 : i1 loc(#loc89)
    %1650 = arith.xori %1647, %true_0 : i1
    %1651 = arith.andi %1626, %1639 : i1
    %1652 = arith.ori %1650, %1651 : i1
    %1653 = arith.ori %1649, %1652 : i1
    %1654 = arith.andi %1651, %1637 : i1
    %1655 = arith.andi %1647, %1654 : i1
    %1656 = arith.andi %1652, %1655 : i1
    %1657 = arith.andi %1647, %1656 : i1
    %1658 = arith.andi %1653, %1657 : i1
    %1659 = arith.ori %1648, %1658 : i1
    %1660 = arith.andi %1642, %1659 : i1
    %c1_i32_62 = arith.constant {acir.source_type = "u32"} 1 : i32
    %1661 = arith.subi %1643, %c1_i32_62 {acir.id = "v5", acir.source_type = "u32"} : i32
    %c3_i32_63 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc90)
    %1662 = arith.andi %1661, %c3_i32_63 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc90)
    %c0_i32_64 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc91)
    %1663 = arith.cmpi eq, %1662, %c0_i32_64 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc91)
    %1664 = arith.xori %1663, %true_0 : i1 loc(#loc91)
    %1665 = arith.andi %1647, %1664 : i1 loc(#loc91)
    %1666 = arith.xori %1665, %true_0 : i1 loc(#loc91)
    %1667 = arith.xori %1665, %true_0 : i1
    %1668 = arith.andi %1653, %1657 : i1
    %1669 = arith.ori %1667, %1668 : i1
    %1670 = arith.andi %1669, %1657 : i1
    %1671 = arith.ori %1666, %1670 : i1
    %1672 = arith.andi %1660, %1671 : i1
    %1673 = arith.cmpi eq, %1662, %c1_i32_62 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc92)
    %1674 = arith.xori %1673, %true_0 : i1 loc(#loc92)
    %1675 = arith.andi %1665, %1674 : i1 loc(#loc92)
    %1676 = arith.xori %1675, %true_0 : i1 loc(#loc92)
    %1677 = arith.xori %1675, %true_0 : i1
    %1678 = arith.andi %1669, %1657 : i1
    %1679 = arith.ori %1677, %1678 : i1
    %1680 = arith.andi %1679, %1657 : i1
    %1681 = arith.ori %1676, %1680 : i1
    %1682 = arith.andi %1672, %1681 : i1
    %c2_i32_65 = arith.constant {acir.source_type = "u32"} 2 : i32
    %1683 = arith.cmpi eq, %1662, %c2_i32_65 {acir.id = "v19", acir.source_type = "bool"} : i32
    %1684 = arith.xori %1683, %true_0 : i1
    %1685 = arith.andi %1675, %1684 : i1
    %1686 = arith.xori %1685, %true_0 : i1
    %1687 = arith.xori %1685, %true_0 : i1 loc(#loc93)
    %1688 = arith.xori %1685, %true_0 : i1
    %1689 = arith.andi %1679, %1657 : i1
    %1690 = arith.ori %1688, %1689 : i1
    %1691 = arith.ori %1687, %1690 : i1
    %1692 = arith.andi %1691, %true : i1 loc(#loc94)
    %1693 = arith.ori %1686, %1692 : i1 loc(#loc94)
    %1694 = arith.andi %1682, %1693 : i1 loc(#loc94)
    %1695 = arith.andi %1675, %1683 : i1
    %1696 = arith.xori %1695, %true_0 : i1
    %1697 = arith.xori %1695, %true_0 : i1 loc(#loc95)
    %1698 = arith.xori %1695, %true_0 : i1 loc(#loc96)
    %1699 = arith.andi %1679, %1657 : i1
    %1700 = arith.ori %1698, %1699 : i1
    %1701 = arith.ori %1697, %1700 : i1
    %1702 = arith.andi %1701, %true : i1 loc(#loc94)
    %1703 = arith.ori %1696, %1702 : i1 loc(#loc94)
    %1704 = arith.andi %1694, %1703 : i1 loc(#loc94)
    %1705 = arith.andi %1665, %1673 : i1
    %1706 = arith.xori %1705, %true_0 : i1
    %1707 = arith.xori %1705, %true_0 : i1 loc(#loc97)
    %1708 = arith.xori %1705, %true_0 : i1 loc(#loc98)
    %1709 = arith.andi %1669, %1657 : i1
    %1710 = arith.ori %1708, %1709 : i1
    %1711 = arith.ori %1707, %1710 : i1
    %1712 = arith.andi %1711, %true : i1 loc(#loc94)
    %1713 = arith.ori %1706, %1712 : i1 loc(#loc94)
    %1714 = arith.andi %1704, %1713 : i1 loc(#loc94)
    %1715 = arith.andi %1647, %1663 : i1
    %1716 = arith.xori %1715, %true_0 : i1
    %1717 = arith.xori %1715, %true_0 : i1 loc(#loc99)
    %1718 = arith.xori %1715, %true_0 : i1 loc(#loc100)
    %1719 = arith.andi %1653, %1657 : i1
    %1720 = arith.ori %1718, %1719 : i1
    %1721 = arith.ori %1717, %1720 : i1
    %1722 = arith.andi %1721, %true : i1 loc(#loc94)
    %1723 = arith.ori %1716, %1722 : i1 loc(#loc94)
    %1724 = arith.andi %1714, %1723 : i1 loc(#loc94)
    %1725 = arith.xori %1647, %true_0 : i1
    %1726 = arith.andi %1691, %true : i1 loc(#loc101)
    %1727 = arith.andi %1685, %1726 : i1 loc(#loc101)
    %1728 = arith.andi %1690, %1727 : i1 loc(#loc101)
    %1729 = arith.andi %1685, %1728 : i1 loc(#loc101)
    %1730 = arith.andi %1701, %true : i1 loc(#loc101)
    %1731 = arith.andi %1695, %1730 : i1 loc(#loc101)
    %1732 = arith.andi %1700, %1731 : i1 loc(#loc101)
    %1733 = arith.andi %1695, %1732 : i1 loc(#loc101)
    %1734 = arith.ori %1729, %1733 : i1 loc(#loc101)
    %1735 = arith.andi %1711, %true : i1 loc(#loc101)
    %1736 = arith.andi %1705, %1735 : i1 loc(#loc101)
    %1737 = arith.andi %1710, %1736 : i1 loc(#loc101)
    %1738 = arith.andi %1705, %1737 : i1 loc(#loc101)
    %1739 = arith.ori %1734, %1738 : i1 loc(#loc101)
    %1740 = arith.andi %1721, %true : i1 loc(#loc101)
    %1741 = arith.andi %1715, %1740 : i1 loc(#loc101)
    %1742 = arith.andi %1720, %1741 : i1 loc(#loc101)
    %1743 = arith.andi %1715, %1742 : i1 loc(#loc101)
    %1744 = arith.ori %1739, %1743 : i1 loc(#loc101)
    %1745 = arith.andi %1652, %1744 : i1 loc(#loc101)
    %1746 = arith.ori %1725, %1745 : i1 loc(#loc101)
    %1747 = arith.andi %1724, %1746 : i1 loc(#loc101)
    %1748 = arith.xori %1647, %true_0 : i1
    %1749 = arith.select %1705, %571, %643 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc102)
    %1750 = arith.select %1695, %497, %1749 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc102)
    %1751 = arith.select %1685, %423, %1750 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc102)
    %1752 = acir.extract %1751 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc102)
    %1753 = arith.xori %1752, %true_0 : i1 loc(#loc102)
    %1754 = arith.andi %1647, %1753 : i1 loc(#loc102)
    %1755 = arith.andi %1652, %1744 : i1
    %1756 = arith.andi %1754, %1755 : i1
    %1757 = arith.andi %1647, %1752 : i1
    %1758 = arith.xori %1757, %true_0 : i1
    %1759 = arith.andi %1652, %1744 : i1
    %1760 = arith.ori %1758, %1759 : i1
    %1761 = arith.andi %1760, %1744 : i1
    %1762 = arith.andi %1757, %1761 : i1
    %1763 = arith.ori %1756, %1762 : i1
    %1764 = arith.andi %1652, %1763 : i1
    %1765 = arith.ori %1748, %1764 : i1
    %1766 = arith.andi %1747, %1765 : i1
    %1767 = arith.xori %1647, %true_0 : i1
    %false_66 = arith.constant {acir.source_type = "bool"} false
    %1768 = acir.extract %1751 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc103)
    %1769 = arith.select %1754, %false_66, %1768 : i1 loc(#loc103)
    %1770 = arith.xori %1769, %true_0 : i1 loc(#loc103)
    %1771 = arith.andi %1647, %1770 : i1 loc(#loc103)
    %1772 = arith.andi %1652, %1763 : i1
    %1773 = arith.andi %1771, %1772 : i1
    %1774 = arith.andi %1647, %1769 : i1
    %1775 = arith.xori %1774, %true_0 : i1
    %1776 = arith.andi %1652, %1763 : i1
    %1777 = arith.ori %1775, %1776 : i1
    %1778 = arith.andi %1744, %1655 : i1 loc(#loc104)
    %1779 = arith.andi %1777, %1778 : i1 loc(#loc104)
    %1780 = arith.andi %1774, %1779 : i1 loc(#loc104)
    %1781 = arith.ori %1773, %1780 : i1 loc(#loc104)
    %1782 = arith.andi %1652, %1781 : i1 loc(#loc104)
    %1783 = arith.ori %1767, %1782 : i1 loc(#loc104)
    %1784 = arith.andi %1766, %1783 : i1 loc(#loc104)
    %1785 = arith.andi %1622, %1645 : i1
    %1786 = arith.xori %1785, %true_0 : i1
    %1787 = arith.xori %1785, %true_0 : i1 loc(#loc105)
    %1788 = arith.andi %1626, %1639 : i1
    %1789 = arith.ori %1787, %1788 : i1
    %1790 = arith.andi %1789, %true : i1 loc(#loc106)
    %1791 = arith.ori %1786, %1790 : i1 loc(#loc106)
    %1792 = arith.andi %1784, %1791 : i1 loc(#loc106)
    %1793 = arith.xori %1382, %true_0 : i1 loc(#loc109)
    %1794 = arith.xori %1382, %true_0 : i1 loc(#loc110)
    %1795 = arith.xori %1382, %true_0 : i1 loc(#loc110)
    %1796 = arith.ori %1795, %1396 : i1 loc(#loc110)
    %1797 = arith.ori %1794, %1796 : i1 loc(#loc110)
    %1798 = acir.current {aspect = "value", resource = "top.rob.tail"} : i32 loc(#loc109)
    %1799 = arith.andi %1797, %true : i1 loc(#loc109)
    %1800 = arith.ori %1793, %1799 : i1 loc(#loc109)
    %1801 = arith.andi %1792, %1800 : i1 loc(#loc109)
    %1802 = arith.xori %1382, %true_0 : i1 loc(#loc111)
    %1803 = acir.current {aspect = "value", resource = "top.rob.head"} : i32 loc(#loc111)
    %1804 = arith.andi %1797, %true : i1 loc(#loc111)
    %1805 = arith.ori %1802, %1804 : i1 loc(#loc111)
    %1806 = arith.andi %1801, %1805 : i1 loc(#loc111)
    %1807 = arith.xori %1382, %true_0 : i1 loc(#loc112)
    %1808 = arith.andi %true, %true : i1 loc(#loc109)
    %1809 = arith.andi %1797, %1808 : i1 loc(#loc109)
    %1810 = arith.andi %1382, %1809 : i1 loc(#loc109)
    %1811 = arith.andi %1796, %1810 : i1 loc(#loc109)
    %1812 = arith.andi %1382, %1811 : i1 loc(#loc109)
    %1813 = arith.subi %1798, %1803 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc109)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc113)
    %1814 = arith.cmpi ult, %1813, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc113)
    %1815 = arith.andi %1812, %1814 : i1 loc(#loc113)
    %1816 = arith.andi %1396, %1815 : i1 loc(#loc113)
    %1817 = arith.ori %1807, %1816 : i1 loc(#loc113)
    %1818 = arith.andi %1806, %1817 : i1 loc(#loc113)
    %1819 = arith.xori %1382, %true_0 : i1 loc(#loc114)
    %1820 = arith.xori %1382, %true_0 : i1 loc(#loc112)
    %1821 = arith.andi %1396, %1815 : i1 loc(#loc112)
    %1822 = arith.ori %1820, %1821 : i1 loc(#loc112)
    %1823 = arith.andi %1822, %true : i1 loc(#loc114)
    %1824 = arith.ori %1819, %1823 : i1 loc(#loc114)
    %1825 = arith.andi %1818, %1824 : i1 loc(#loc114)
    %1826 = arith.xori %1382, %true_0 : i1
    %1827 = arith.andi %1822, %true : i1
    %1828 = arith.ori %1826, %1827 : i1
    %1829 = arith.andi %1825, %1828 : i1
    %c1_i32_67 = arith.constant {acir.source_type = "u32"} 1 : i32
    %1830 = arith.subi %1798, %c1_i32_67 {acir.id = "v8", acir.source_type = "u32"} : i32
    %c3_i32_68 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc115)
    %1831 = arith.andi %1830, %c3_i32_68 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc115)
    %c0_i32_69 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc116)
    %1832 = arith.cmpi eq, %1831, %c0_i32_69 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc116)
    %1833 = arith.xori %1832, %true_0 : i1 loc(#loc116)
    %1834 = arith.andi %1382, %1833 : i1 loc(#loc116)
    %1835 = arith.xori %1834, %true_0 : i1 loc(#loc116)
    %1836 = arith.xori %1834, %true_0 : i1
    %1837 = arith.andi %1822, %true : i1
    %1838 = arith.ori %1836, %1837 : i1
    %1839 = arith.andi %1838, %true : i1
    %1840 = arith.ori %1835, %1839 : i1
    %1841 = arith.andi %1829, %1840 : i1
    %1842 = arith.cmpi eq, %1831, %c1_i32_67 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc117)
    %1843 = arith.xori %1842, %true_0 : i1 loc(#loc117)
    %1844 = arith.andi %1834, %1843 : i1 loc(#loc117)
    %1845 = arith.xori %1844, %true_0 : i1 loc(#loc117)
    %1846 = arith.xori %1844, %true_0 : i1
    %1847 = arith.andi %1838, %true : i1
    %1848 = arith.ori %1846, %1847 : i1
    %1849 = arith.andi %1848, %true : i1
    %1850 = arith.ori %1845, %1849 : i1
    %1851 = arith.andi %1841, %1850 : i1
    %c2_i32_70 = arith.constant {acir.source_type = "u32"} 2 : i32
    %1852 = arith.cmpi eq, %1831, %c2_i32_70 {acir.id = "v20", acir.source_type = "bool"} : i32
    %1853 = arith.xori %1852, %true_0 : i1
    %1854 = arith.andi %1844, %1853 : i1
    %1855 = arith.xori %1854, %true_0 : i1
    %1856 = arith.xori %1854, %true_0 : i1 loc(#loc118)
    %1857 = arith.xori %1854, %true_0 : i1 loc(#loc118)
    %1858 = arith.xori %1854, %true_0 : i1
    %1859 = arith.andi %1848, %true : i1
    %1860 = arith.ori %1858, %1859 : i1
    %1861 = arith.ori %1857, %1860 : i1
    %1862 = arith.ori %1856, %1861 : i1
    %1863 = arith.andi %1862, %true : i1 loc(#loc119)
    %1864 = arith.ori %1855, %1863 : i1 loc(#loc119)
    %1865 = arith.andi %1851, %1864 : i1 loc(#loc119)
    %1866 = arith.xori %1854, %true_0 : i1 loc(#loc120)
    %1867 = arith.andi %1862, %true : i1 loc(#loc121)
    %1868 = arith.andi %1854, %1867 : i1 loc(#loc121)
    %1869 = arith.andi %1861, %1868 : i1 loc(#loc121)
    %1870 = arith.andi %1854, %1869 : i1 loc(#loc121)
    %1871 = acir.extract %423 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
    %1872 = acir.unary %1871 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc119)
    %1873 = arith.andi %1870, %1872 : i1 loc(#loc119)
    %1874 = arith.andi %1860, %1873 : i1 loc(#loc119)
    %1875 = arith.ori %1866, %1874 : i1 loc(#loc119)
    %1876 = arith.andi %1865, %1875 : i1 loc(#loc119)
    %1877 = arith.xori %1854, %true_0 : i1 loc(#loc122)
    %1878 = arith.xori %1854, %true_0 : i1 loc(#loc120)
    %1879 = arith.andi %1860, %1873 : i1 loc(#loc120)
    %1880 = arith.ori %1878, %1879 : i1 loc(#loc120)
    %1881 = arith.andi %1879, %true : i1 loc(#loc120)
    %1882 = arith.andi %1854, %1881 : i1 loc(#loc120)
    %1883 = arith.andi %1821, %1400 : i1 loc(#loc112)
    %1884 = arith.andi %1382, %1883 : i1 loc(#loc112)
    %1885 = arith.andi %1879, %1884 : i1 loc(#loc112)
    %1886 = arith.andi %1854, %1885 : i1 loc(#loc112)
    %1887 = arith.andi %1882, %1886 : i1 loc(#loc112)
    %1888 = arith.andi %1880, %1887 : i1 loc(#loc112)
    %1889 = arith.ori %1877, %1888 : i1 loc(#loc112)
    %1890 = arith.andi %1876, %1889 : i1 loc(#loc112)
    %1891 = arith.andi %1844, %1852 : i1
    %1892 = arith.xori %1891, %true_0 : i1
    %1893 = arith.xori %1891, %true_0 : i1 loc(#loc123)
    %1894 = arith.xori %1891, %true_0 : i1 loc(#loc123)
    %1895 = arith.xori %1891, %true_0 : i1
    %1896 = arith.andi %1848, %true : i1
    %1897 = arith.ori %1895, %1896 : i1
    %1898 = arith.ori %1894, %1897 : i1
    %1899 = arith.ori %1893, %1898 : i1
    %1900 = arith.andi %1899, %true : i1 loc(#loc119)
    %1901 = arith.ori %1892, %1900 : i1 loc(#loc119)
    %1902 = arith.andi %1890, %1901 : i1 loc(#loc119)
    %1903 = arith.xori %1891, %true_0 : i1 loc(#loc120)
    %1904 = arith.andi %1899, %true : i1 loc(#loc121)
    %1905 = arith.andi %1891, %1904 : i1 loc(#loc121)
    %1906 = arith.andi %1898, %1905 : i1 loc(#loc121)
    %1907 = arith.andi %1891, %1906 : i1 loc(#loc121)
    %1908 = acir.extract %497 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
    %1909 = acir.unary %1908 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc119)
    %1910 = arith.andi %1907, %1909 : i1 loc(#loc119)
    %1911 = arith.andi %1897, %1910 : i1 loc(#loc119)
    %1912 = arith.ori %1903, %1911 : i1 loc(#loc119)
    %1913 = arith.andi %1902, %1912 : i1 loc(#loc119)
    %1914 = arith.xori %1891, %true_0 : i1 loc(#loc122)
    %1915 = arith.xori %1891, %true_0 : i1 loc(#loc120)
    %1916 = arith.andi %1897, %1910 : i1 loc(#loc120)
    %1917 = arith.ori %1915, %1916 : i1 loc(#loc120)
    %1918 = arith.andi %1916, %true : i1 loc(#loc120)
    %1919 = arith.andi %1891, %1918 : i1 loc(#loc120)
    %1920 = arith.andi %1916, %1884 : i1 loc(#loc120)
    %1921 = arith.andi %1891, %1920 : i1 loc(#loc120)
    %1922 = arith.andi %1919, %1921 : i1 loc(#loc120)
    %1923 = arith.andi %1917, %1922 : i1 loc(#loc120)
    %1924 = arith.ori %1914, %1923 : i1 loc(#loc120)
    %1925 = arith.andi %1913, %1924 : i1 loc(#loc120)
    %1926 = arith.andi %1834, %1842 : i1
    %1927 = arith.xori %1926, %true_0 : i1
    %1928 = arith.xori %1926, %true_0 : i1 loc(#loc124)
    %1929 = arith.xori %1926, %true_0 : i1 loc(#loc124)
    %1930 = arith.xori %1926, %true_0 : i1
    %1931 = arith.andi %1838, %true : i1
    %1932 = arith.ori %1930, %1931 : i1
    %1933 = arith.ori %1929, %1932 : i1
    %1934 = arith.ori %1928, %1933 : i1
    %1935 = arith.andi %1934, %true : i1 loc(#loc119)
    %1936 = arith.ori %1927, %1935 : i1 loc(#loc119)
    %1937 = arith.andi %1925, %1936 : i1 loc(#loc119)
    %1938 = arith.xori %1926, %true_0 : i1 loc(#loc120)
    %1939 = arith.andi %1934, %true : i1 loc(#loc121)
    %1940 = arith.andi %1926, %1939 : i1 loc(#loc121)
    %1941 = arith.andi %1933, %1940 : i1 loc(#loc121)
    %1942 = arith.andi %1926, %1941 : i1 loc(#loc121)
    %1943 = acir.extract %571 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
    %1944 = acir.unary %1943 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc119)
    %1945 = arith.andi %1942, %1944 : i1 loc(#loc119)
    %1946 = arith.andi %1932, %1945 : i1 loc(#loc119)
    %1947 = arith.ori %1938, %1946 : i1 loc(#loc119)
    %1948 = arith.andi %1937, %1947 : i1 loc(#loc119)
    %1949 = arith.xori %1926, %true_0 : i1 loc(#loc122)
    %1950 = arith.xori %1926, %true_0 : i1 loc(#loc120)
    %1951 = arith.andi %1932, %1945 : i1 loc(#loc120)
    %1952 = arith.ori %1950, %1951 : i1 loc(#loc120)
    %1953 = arith.andi %1951, %true : i1 loc(#loc120)
    %1954 = arith.andi %1926, %1953 : i1 loc(#loc120)
    %1955 = arith.andi %1951, %1884 : i1 loc(#loc120)
    %1956 = arith.andi %1926, %1955 : i1 loc(#loc120)
    %1957 = arith.andi %1954, %1956 : i1 loc(#loc120)
    %1958 = arith.andi %1952, %1957 : i1 loc(#loc120)
    %1959 = arith.ori %1949, %1958 : i1 loc(#loc120)
    %1960 = arith.andi %1948, %1959 : i1 loc(#loc120)
    %1961 = arith.andi %1382, %1832 : i1
    %1962 = arith.xori %1961, %true_0 : i1
    %1963 = arith.xori %1961, %true_0 : i1 loc(#loc125)
    %1964 = arith.xori %1961, %true_0 : i1 loc(#loc125)
    %1965 = arith.xori %1961, %true_0 : i1
    %1966 = arith.andi %1822, %true : i1
    %1967 = arith.ori %1965, %1966 : i1
    %1968 = arith.ori %1964, %1967 : i1
    %1969 = arith.ori %1963, %1968 : i1
    %1970 = arith.andi %1969, %true : i1 loc(#loc119)
    %1971 = arith.ori %1962, %1970 : i1 loc(#loc119)
    %1972 = arith.andi %1960, %1971 : i1 loc(#loc119)
    %1973 = arith.xori %1961, %true_0 : i1 loc(#loc120)
    %1974 = arith.andi %1969, %true : i1 loc(#loc121)
    %1975 = arith.andi %1961, %1974 : i1 loc(#loc121)
    %1976 = arith.andi %1968, %1975 : i1 loc(#loc121)
    %1977 = arith.andi %1961, %1976 : i1 loc(#loc121)
    %1978 = acir.extract %643 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
    %1979 = acir.unary %1978 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc119)
    %1980 = arith.andi %1977, %1979 : i1 loc(#loc119)
    %1981 = arith.andi %1967, %1980 : i1 loc(#loc119)
    %1982 = arith.ori %1973, %1981 : i1 loc(#loc119)
    %1983 = arith.andi %1972, %1982 : i1 loc(#loc119)
    %1984 = arith.xori %1961, %true_0 : i1 loc(#loc122)
    %1985 = arith.xori %1961, %true_0 : i1 loc(#loc120)
    %1986 = arith.andi %1967, %1980 : i1 loc(#loc120)
    %1987 = arith.ori %1985, %1986 : i1 loc(#loc120)
    %1988 = arith.andi %1986, %true : i1 loc(#loc120)
    %1989 = arith.andi %1961, %1988 : i1 loc(#loc120)
    %1990 = arith.andi %1986, %1884 : i1 loc(#loc120)
    %1991 = arith.andi %1961, %1990 : i1 loc(#loc120)
    %1992 = arith.andi %1989, %1991 : i1 loc(#loc120)
    %1993 = arith.andi %1987, %1992 : i1 loc(#loc120)
    %1994 = arith.ori %1984, %1993 : i1 loc(#loc120)
    %1995 = arith.andi %1983, %1994 : i1 loc(#loc120)
    %1996 = arith.xori %1382, %true_0 : i1 loc(#loc126)
    %1997 = arith.andi %1822, %true : i1 loc(#loc126)
    %1998 = arith.ori %1996, %1997 : i1 loc(#loc126)
    %1999 = arith.andi %1995, %1998 : i1 loc(#loc126)
    %2000 = arith.xori %1382, %true_0 : i1 loc(#loc127)
    %2001 = arith.xori %1382, %true_0 : i1 loc(#loc128)
    %2002 = arith.ori %2001, %1396 : i1 loc(#loc128)
    %2003 = arith.andi %1396, %1400 : i1 loc(#loc128)
    %2004 = arith.andi %1382, %2003 : i1 loc(#loc128)
    %2005 = arith.andi %2002, %2004 : i1 loc(#loc128)
    %2006 = arith.ori %2000, %2005 : i1 loc(#loc128)
    %2007 = arith.andi %1999, %2006 : i1 loc(#loc128)
    %2008 = arith.xori %1382, %true_0 : i1
    %2009 = arith.andi %2002, %2004 : i1
    %2010 = arith.ori %2008, %2009 : i1
    %2011 = arith.andi %2007, %2010 : i1
    %2012 = acir.extract %1398 {acir.id = "v11", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc129)
    %c0_i32_71 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc130)
    %2013 = arith.cmpi ne, %2012, %c0_i32_71 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc130)
    %2014 = arith.andi %1382, %2013 : i1 loc(#loc130)
    %2015 = arith.xori %2014, %true_0 : i1 loc(#loc130)
    %2016 = arith.xori %2014, %true_0 : i1
    %2017 = arith.andi %2002, %2004 : i1
    %2018 = arith.ori %2016, %2017 : i1
    %2019 = arith.andi %2018, %true : i1 loc(#loc131)
    %2020 = arith.ori %2015, %2019 : i1 loc(#loc131)
    %2021 = arith.andi %2011, %2020 : i1 loc(#loc131)
    %2022 = arith.xori %2014, %true_0 : i1 loc(#loc131)
    %2023 = arith.andi %2018, %true : i1 loc(#loc131)
    %2024 = arith.ori %2022, %2023 : i1 loc(#loc131)
    %2025 = arith.andi %2021, %2024 : i1 loc(#loc131)
    %2026 = arith.xori %2014, %true_0 : i1 loc(#loc131)
    %2027 = arith.andi %1822, %true : i1 loc(#loc132)
    %2028 = arith.andi %1382, %2027 : i1 loc(#loc132)
    %2029 = arith.andi %2028, %1815 : i1 loc(#loc132)
    %2030 = arith.andi %1396, %2029 : i1 loc(#loc132)
    %2031 = arith.andi %1382, %2030 : i1 loc(#loc132)
    %2032 = arith.andi %true, %2031 : i1 loc(#loc132)
    %2033 = arith.andi %2032, %2004 : i1 loc(#loc132)
    %2034 = arith.andi %2018, %2033 : i1 loc(#loc132)
    %2035 = arith.ori %2026, %2034 : i1 loc(#loc132)
    %2036 = arith.andi %2025, %2035 : i1 loc(#loc132)
    %2037 = arith.xori %1382, %true_0 : i1 loc(#loc133)
    %2038 = arith.andi %1396, %true : i1 loc(#loc133)
    %2039 = arith.ori %2037, %2038 : i1 loc(#loc133)
    %2040 = arith.andi %2036, %2039 : i1 loc(#loc133)
    %2041 = arith.xori %1382, %true_0 : i1
    %2042 = arith.andi %1396, %true : i1
    %2043 = arith.ori %2041, %2042 : i1
    %2044 = arith.andi %2040, %2043 : i1
    %2045 = acir.extract %0 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc133)
    %2046 = acir.unary %2045 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc133)
    %2047 = arith.xori %2046, %true_0 : i1 loc(#loc133)
    %2048 = arith.andi %1382, %2047 : i1 loc(#loc133)
    %2049 = arith.xori %2048, %true_0 : i1 loc(#loc133)
    %2050 = arith.xori %2048, %true_0 : i1 loc(#loc134)
    %2051 = arith.xori %2048, %true_0 : i1 loc(#loc134)
    %2052 = arith.xori %2048, %true_0 : i1
    %2053 = arith.andi %1396, %true : i1
    %2054 = arith.ori %2052, %2053 : i1
    %2055 = arith.ori %2051, %2054 : i1
    %2056 = arith.ori %2050, %2055 : i1
    %2057 = arith.andi %2056, %true : i1 loc(#loc135)
    %2058 = arith.ori %2049, %2057 : i1 loc(#loc135)
    %2059 = arith.andi %2044, %2058 : i1 loc(#loc135)
    %2060 = arith.xori %2048, %true_0 : i1 loc(#loc136)
    %2061 = arith.andi %2056, %true : i1 loc(#loc137)
    %2062 = arith.andi %2048, %2061 : i1 loc(#loc137)
    %2063 = arith.andi %2055, %2062 : i1 loc(#loc137)
    %2064 = arith.andi %2048, %2063 : i1 loc(#loc137)
    %2065 = acir.extract %46 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc135)
    %2066 = acir.unary %2065 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
    %2067 = arith.andi %2064, %2066 : i1 loc(#loc135)
    %2068 = arith.andi %2054, %2067 : i1 loc(#loc135)
    %2069 = arith.ori %2060, %2068 : i1 loc(#loc135)
    %2070 = arith.andi %2059, %2069 : i1 loc(#loc135)
    %2071 = arith.xori %2048, %true_0 : i1 loc(#loc138)
    %2072 = arith.xori %2048, %true_0 : i1 loc(#loc136)
    %2073 = arith.andi %2054, %2067 : i1 loc(#loc136)
    %2074 = arith.ori %2072, %2073 : i1 loc(#loc136)
    %2075 = arith.andi %1400, %2029 : i1 loc(#loc139)
    %2076 = arith.andi %2075, %1400 : i1 loc(#loc139)
    %2077 = acir.extract %1564 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc104)
    %2078 = arith.cmpi eq, %2077, %1456 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc104)
    %2079 = arith.select %1584, %false_60, %2078 : i1 loc(#loc104)
    %2080 = arith.xori %2079, %true_0 : i1 loc(#loc104)
    %2081 = arith.andi %1460, %2080 : i1 loc(#loc104)
    %2082 = arith.xori %2081, %true_0 : i1
    %2083 = arith.andi %1465, %1594 : i1
    %2084 = arith.ori %2082, %2083 : i1
    %2085 = arith.andi %2083, %1468 : i1
    %2086 = arith.andi %2081, %2085 : i1
    %2087 = arith.andi %2084, %2086 : i1
    %2088 = arith.andi %2081, %2087 : i1
    %2089 = arith.andi %1460, %2079 : i1
    %2090 = arith.xori %2089, %true_0 : i1 loc(#loc140)
    %2091 = arith.andi %1465, %1594 : i1
    %2092 = arith.ori %2090, %2091 : i1
    %2093 = arith.andi %1468, %1557 : i1 loc(#loc141)
    %2094 = arith.andi %2092, %2093 : i1 loc(#loc141)
    %2095 = arith.andi %2089, %2094 : i1 loc(#loc141)
    %2096 = arith.ori %2088, %2095 : i1 loc(#loc141)
    %2097 = arith.andi %true, %1449 : i1 loc(#loc106)
    %2098 = arith.andi %1450, %2097 : i1 loc(#loc106)
    %2099 = arith.andi %1602, %2098 : i1 loc(#loc106)
    %2100 = arith.andi %1598, %2099 : i1 loc(#loc106)
    %2101 = arith.ori %2096, %2100 : i1 loc(#loc106)
    %2102 = arith.andi %1382, %1431 : i1
    %2103 = arith.xori %2102, %true_0 : i1 loc(#loc142)
    %2104 = arith.andi %1420, %1422 : i1
    %2105 = arith.ori %2103, %2104 : i1
    %2106 = arith.andi %2102, %2105 : i1
    %2107 = arith.ori %2101, %2106 : i1
    %2108 = arith.andi %2076, %2107 : i1
    %2109 = acir.extract %1751 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc104)
    %2110 = arith.cmpi eq, %2109, %1643 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc104)
    %2111 = arith.select %1771, %false_66, %2110 : i1 loc(#loc104)
    %2112 = arith.xori %2111, %true_0 : i1 loc(#loc104)
    %2113 = arith.andi %1647, %2112 : i1 loc(#loc104)
    %2114 = arith.xori %2113, %true_0 : i1
    %2115 = arith.andi %1652, %1781 : i1
    %2116 = arith.ori %2114, %2115 : i1
    %2117 = arith.andi %2115, %1655 : i1
    %2118 = arith.andi %2113, %2117 : i1
    %2119 = arith.andi %2116, %2118 : i1
    %2120 = arith.andi %2113, %2119 : i1
    %2121 = arith.andi %1647, %2111 : i1
    %2122 = arith.xori %2121, %true_0 : i1 loc(#loc140)
    %2123 = arith.andi %1652, %1781 : i1
    %2124 = arith.ori %2122, %2123 : i1
    %2125 = arith.andi %1655, %1744 : i1 loc(#loc141)
    %2126 = arith.andi %2124, %2125 : i1 loc(#loc141)
    %2127 = arith.andi %2121, %2126 : i1 loc(#loc141)
    %2128 = arith.ori %2120, %2127 : i1 loc(#loc141)
    %2129 = arith.andi %true, %1636 : i1 loc(#loc106)
    %2130 = arith.andi %1637, %2129 : i1 loc(#loc106)
    %2131 = arith.andi %1789, %2130 : i1 loc(#loc106)
    %2132 = arith.andi %1785, %2131 : i1 loc(#loc106)
    %2133 = arith.ori %2128, %2132 : i1 loc(#loc106)
    %2134 = arith.andi %1382, %1620 : i1
    %2135 = arith.xori %2134, %true_0 : i1 loc(#loc142)
    %2136 = arith.andi %1609, %1611 : i1
    %2137 = arith.ori %2135, %2136 : i1
    %2138 = arith.andi %2134, %2137 : i1
    %2139 = arith.ori %2133, %2138 : i1
    %2140 = arith.andi %2108, %2139 : i1
    %2141 = arith.andi %2073, %2140 : i1
    %2142 = arith.andi %2048, %2141 : i1
    %2143 = arith.andi %2074, %2142 : i1
    %2144 = arith.ori %2071, %2143 : i1
    %2145 = arith.andi %2070, %2144 : i1
    %2146 = arith.andi %1382, %2046 : i1
    %2147 = arith.xori %2146, %true_0 : i1
    %2148 = arith.xori %2146, %true_0 : i1 loc(#loc143)
    %2149 = arith.xori %2146, %true_0 : i1 loc(#loc143)
    %2150 = arith.xori %2146, %true_0 : i1
    %2151 = arith.andi %1396, %true : i1
    %2152 = arith.ori %2150, %2151 : i1
    %2153 = arith.ori %2149, %2152 : i1
    %2154 = arith.ori %2148, %2153 : i1
    %2155 = arith.andi %2154, %true : i1 loc(#loc135)
    %2156 = arith.ori %2147, %2155 : i1 loc(#loc135)
    %2157 = arith.andi %2145, %2156 : i1 loc(#loc135)
    %2158 = arith.xori %2146, %true_0 : i1 loc(#loc136)
    %2159 = arith.andi %2154, %true : i1 loc(#loc137)
    %2160 = arith.andi %2146, %2159 : i1 loc(#loc137)
    %2161 = arith.andi %2153, %2160 : i1 loc(#loc137)
    %2162 = arith.andi %2146, %2161 : i1 loc(#loc137)
    %2163 = acir.extract %0 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc135)
    %2164 = acir.unary %2163 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
    %2165 = arith.andi %2162, %2164 : i1 loc(#loc135)
    %2166 = arith.andi %2152, %2165 : i1 loc(#loc135)
    %2167 = arith.ori %2158, %2166 : i1 loc(#loc135)
    %2168 = arith.andi %2157, %2167 : i1 loc(#loc135)
    %2169 = arith.xori %2146, %true_0 : i1 loc(#loc138)
    %2170 = arith.xori %2146, %true_0 : i1 loc(#loc136)
    %2171 = arith.andi %2152, %2165 : i1 loc(#loc136)
    %2172 = arith.ori %2170, %2171 : i1 loc(#loc136)
    %2173 = arith.andi %2171, %2140 : i1 loc(#loc136)
    %2174 = arith.andi %2146, %2173 : i1 loc(#loc136)
    %2175 = arith.andi %2172, %2174 : i1 loc(#loc136)
    %2176 = arith.ori %2169, %2175 : i1 loc(#loc136)
    %2177 = arith.andi %2168, %2176 : i1 loc(#loc136)
    %2178 = acir.fire "top.dispatch.dispatch#6" %2177 {successors = [], txn = 6 : i64} loc(#loc136)
    %2179 = arith.andi %true, %true : i1 loc(#loc144)
    %c1_i32_72 = arith.constant {acir.source_type = "u32"} 1 : i32
    %2180 = arith.subi %1803, %c1_i32_72 {acir.id = "v6", acir.source_type = "u32"} : i32
    %c3_i32_73 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc145)
    %2181 = arith.andi %2180, %c3_i32_73 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc145)
    %c0_i32_74 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc146)
    %2182 = arith.cmpi eq, %2181, %c0_i32_74 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc146)
    %2183 = arith.xori %2182, %true_0 : i1 loc(#loc146)
    %2184 = arith.xori %2183, %true_0 : i1 loc(#loc146)
    %2185 = arith.xori %2183, %true_0 : i1
    %2186 = arith.ori %2185, %true : i1
    %2187 = arith.andi %2186, %true : i1
    %2188 = arith.ori %2184, %2187 : i1
    %2189 = arith.andi %2179, %2188 : i1
    %2190 = arith.cmpi eq, %2181, %c1_i32_72 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc147)
    %2191 = arith.xori %2190, %true_0 : i1 loc(#loc147)
    %2192 = arith.andi %2183, %2191 : i1 loc(#loc147)
    %2193 = arith.xori %2192, %true_0 : i1 loc(#loc147)
    %2194 = arith.xori %2192, %true_0 : i1
    %2195 = arith.andi %2186, %true : i1
    %2196 = arith.ori %2194, %2195 : i1
    %2197 = arith.andi %2196, %true : i1
    %2198 = arith.ori %2193, %2197 : i1
    %2199 = arith.andi %2189, %2198 : i1
    %c2_i32_75 = arith.constant {acir.source_type = "u32"} 2 : i32
    %2200 = arith.cmpi eq, %2181, %c2_i32_75 {acir.id = "v18", acir.source_type = "bool"} : i32
    %2201 = arith.xori %2200, %true_0 : i1
    %2202 = arith.andi %2192, %2201 : i1
    %2203 = arith.xori %2202, %true_0 : i1
    %2204 = arith.xori %2202, %true_0 : i1 loc(#loc148)
    %2205 = arith.xori %2202, %true_0 : i1 loc(#loc148)
    %2206 = arith.xori %2202, %true_0 : i1
    %2207 = arith.andi %2196, %true : i1
    %2208 = arith.ori %2206, %2207 : i1
    %2209 = arith.ori %2205, %2208 : i1
    %2210 = arith.ori %2204, %2209 : i1
    %2211 = arith.andi %2210, %true : i1 loc(#loc149)
    %2212 = arith.ori %2203, %2211 : i1 loc(#loc149)
    %2213 = arith.andi %2199, %2212 : i1 loc(#loc149)
    %2214 = arith.xori %2202, %true_0 : i1
    %2215 = arith.andi %2210, %true : i1
    %2216 = arith.ori %2214, %2215 : i1
    %2217 = arith.andi %2213, %2216 : i1
    %2218 = arith.xori %2202, %true_0 : i1
    %2219 = arith.xori %2202, %true_0 : i1
    %2220 = arith.ori %2219, %2210 : i1
    %2221 = acir.extract %423 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc150)
    %2222 = arith.xori %2221, %true_0 : i1 loc(#loc150)
    %2223 = arith.andi %2202, %2222 : i1 loc(#loc150)
    %2224 = arith.xori %2223, %true_0 : i1
    %2225 = arith.andi %2210, %true : i1
    %2226 = arith.ori %2224, %2225 : i1
    %2227 = arith.andi %2223, %2226 : i1
    %2228 = arith.andi %2202, %2221 : i1
    %2229 = arith.xori %2228, %true_0 : i1
    %2230 = arith.andi %2210, %true : i1
    %2231 = arith.ori %2229, %2230 : i1
    %2232 = arith.andi %2231, %true : i1
    %2233 = arith.andi %2228, %2232 : i1
    %2234 = arith.ori %2227, %2233 : i1
    %2235 = arith.andi %2220, %2234 : i1
    %2236 = arith.ori %2218, %2235 : i1
    %2237 = arith.andi %2217, %2236 : i1
    %2238 = arith.xori %2202, %true_0 : i1 loc(#loc151)
    %2239 = arith.xori %2202, %true_0 : i1
    %2240 = arith.ori %2239, %2220 : i1
    %false_76 = arith.constant {acir.source_type = "bool"} false
    %2241 = acir.extract %423 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %2242 = arith.select %2223, %false_76, %2241 : i1 loc(#loc152)
    %2243 = arith.xori %2242, %true_0 : i1 loc(#loc152)
    %2244 = arith.andi %2202, %2243 : i1 loc(#loc152)
    %2245 = arith.xori %2244, %true_0 : i1
    %2246 = arith.andi %2220, %2234 : i1
    %2247 = arith.ori %2245, %2246 : i1
    %2248 = arith.andi %2244, %2247 : i1
    %2249 = arith.andi %2202, %2242 : i1
    %2250 = arith.xori %2249, %true_0 : i1
    %2251 = arith.andi %2220, %2234 : i1
    %2252 = arith.ori %2250, %2251 : i1
    %2253 = arith.andi %2208, %true : i1 loc(#loc148)
    %2254 = arith.andi %2202, %2253 : i1 loc(#loc148)
    %2255 = arith.andi %2209, %2254 : i1 loc(#loc148)
    %2256 = arith.andi %2202, %2255 : i1 loc(#loc148)
    %2257 = arith.andi %true, %2256 : i1 loc(#loc148)
    %2258 = arith.andi %2252, %2257 : i1 loc(#loc148)
    %2259 = arith.andi %2249, %2258 : i1 loc(#loc148)
    %2260 = arith.ori %2248, %2259 : i1 loc(#loc148)
    %2261 = arith.andi %2240, %2260 : i1 loc(#loc148)
    %2262 = arith.andi %2202, %2261 : i1 loc(#loc148)
    %2263 = arith.andi %2209, %2262 : i1 loc(#loc148)
    %2264 = arith.andi %2202, %2263 : i1 loc(#loc148)
    %2265 = acir.extract %423 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
    %2266 = arith.cmpi eq, %2265, %1803 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc153)
    %2267 = arith.select %2244, %false_76, %2266 : i1 loc(#loc153)
    %2268 = arith.andi %2264, %2267 : i1 loc(#loc153)
    %2269 = arith.andi %2208, %2268 : i1 loc(#loc153)
    %2270 = arith.ori %2238, %2269 : i1 loc(#loc153)
    %2271 = arith.andi %2237, %2270 : i1 loc(#loc153)
    %2272 = arith.xori %2202, %true_0 : i1 loc(#loc154)
    %2273 = arith.xori %2202, %true_0 : i1 loc(#loc151)
    %2274 = arith.andi %2208, %2268 : i1 loc(#loc151)
    %2275 = arith.ori %2273, %2274 : i1 loc(#loc151)
    %2276 = arith.andi %2275, %true : i1 loc(#loc154)
    %2277 = arith.ori %2272, %2276 : i1 loc(#loc154)
    %2278 = arith.andi %2271, %2277 : i1 loc(#loc154)
    %2279 = arith.xori %2202, %true_0 : i1 loc(#loc155)
    %2280 = arith.ori %2279, %2275 : i1 loc(#loc155)
    %2281 = arith.andi %2278, %2280 : i1 loc(#loc155)
    %2282 = arith.andi %2192, %2200 : i1
    %2283 = arith.xori %2282, %true_0 : i1
    %2284 = arith.xori %2282, %true_0 : i1 loc(#loc156)
    %2285 = arith.xori %2282, %true_0 : i1 loc(#loc156)
    %2286 = arith.xori %2282, %true_0 : i1
    %2287 = arith.andi %2196, %true : i1
    %2288 = arith.ori %2286, %2287 : i1
    %2289 = arith.ori %2285, %2288 : i1
    %2290 = arith.ori %2284, %2289 : i1
    %2291 = arith.andi %2290, %true : i1 loc(#loc149)
    %2292 = arith.ori %2283, %2291 : i1 loc(#loc149)
    %2293 = arith.andi %2281, %2292 : i1 loc(#loc149)
    %2294 = arith.xori %2282, %true_0 : i1
    %2295 = arith.andi %2290, %true : i1
    %2296 = arith.ori %2294, %2295 : i1
    %2297 = arith.andi %2293, %2296 : i1
    %2298 = arith.xori %2282, %true_0 : i1
    %2299 = arith.xori %2282, %true_0 : i1
    %2300 = arith.ori %2299, %2290 : i1
    %2301 = acir.extract %497 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc150)
    %2302 = arith.xori %2301, %true_0 : i1 loc(#loc150)
    %2303 = arith.andi %2282, %2302 : i1 loc(#loc150)
    %2304 = arith.xori %2303, %true_0 : i1
    %2305 = arith.andi %2290, %true : i1
    %2306 = arith.ori %2304, %2305 : i1
    %2307 = arith.andi %2303, %2306 : i1
    %2308 = arith.andi %2282, %2301 : i1
    %2309 = arith.xori %2308, %true_0 : i1
    %2310 = arith.andi %2290, %true : i1
    %2311 = arith.ori %2309, %2310 : i1
    %2312 = arith.andi %2311, %true : i1
    %2313 = arith.andi %2308, %2312 : i1
    %2314 = arith.ori %2307, %2313 : i1
    %2315 = arith.andi %2300, %2314 : i1
    %2316 = arith.ori %2298, %2315 : i1
    %2317 = arith.andi %2297, %2316 : i1
    %2318 = arith.xori %2282, %true_0 : i1 loc(#loc151)
    %2319 = arith.xori %2282, %true_0 : i1
    %2320 = arith.ori %2319, %2300 : i1
    %false_77 = arith.constant {acir.source_type = "bool"} false
    %2321 = acir.extract %497 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %2322 = arith.select %2303, %false_77, %2321 : i1 loc(#loc152)
    %2323 = arith.xori %2322, %true_0 : i1 loc(#loc152)
    %2324 = arith.andi %2282, %2323 : i1 loc(#loc152)
    %2325 = arith.xori %2324, %true_0 : i1
    %2326 = arith.andi %2300, %2314 : i1
    %2327 = arith.ori %2325, %2326 : i1
    %2328 = arith.andi %2324, %2327 : i1
    %2329 = arith.andi %2282, %2322 : i1
    %2330 = arith.xori %2329, %true_0 : i1
    %2331 = arith.andi %2300, %2314 : i1
    %2332 = arith.ori %2330, %2331 : i1
    %2333 = arith.andi %2288, %true : i1 loc(#loc156)
    %2334 = arith.andi %2282, %2333 : i1 loc(#loc156)
    %2335 = arith.andi %2289, %2334 : i1 loc(#loc156)
    %2336 = arith.andi %2282, %2335 : i1 loc(#loc156)
    %2337 = arith.andi %true, %2336 : i1 loc(#loc156)
    %2338 = arith.andi %2332, %2337 : i1 loc(#loc156)
    %2339 = arith.andi %2329, %2338 : i1 loc(#loc156)
    %2340 = arith.ori %2328, %2339 : i1 loc(#loc156)
    %2341 = arith.andi %2320, %2340 : i1 loc(#loc156)
    %2342 = arith.andi %2282, %2341 : i1 loc(#loc156)
    %2343 = arith.andi %2289, %2342 : i1 loc(#loc156)
    %2344 = arith.andi %2282, %2343 : i1 loc(#loc156)
    %2345 = acir.extract %497 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
    %2346 = arith.cmpi eq, %2345, %1803 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc153)
    %2347 = arith.select %2324, %false_77, %2346 : i1 loc(#loc153)
    %2348 = arith.andi %2344, %2347 : i1 loc(#loc153)
    %2349 = arith.andi %2288, %2348 : i1 loc(#loc153)
    %2350 = arith.ori %2318, %2349 : i1 loc(#loc153)
    %2351 = arith.andi %2317, %2350 : i1 loc(#loc153)
    %2352 = arith.xori %2282, %true_0 : i1 loc(#loc154)
    %2353 = arith.xori %2282, %true_0 : i1 loc(#loc151)
    %2354 = arith.andi %2288, %2348 : i1 loc(#loc151)
    %2355 = arith.ori %2353, %2354 : i1 loc(#loc151)
    %2356 = arith.andi %2355, %true : i1 loc(#loc154)
    %2357 = arith.ori %2352, %2356 : i1 loc(#loc154)
    %2358 = arith.andi %2351, %2357 : i1 loc(#loc154)
    %2359 = arith.xori %2282, %true_0 : i1 loc(#loc155)
    %2360 = arith.ori %2359, %2355 : i1 loc(#loc155)
    %2361 = arith.andi %2358, %2360 : i1 loc(#loc155)
    %2362 = arith.andi %2183, %2190 : i1
    %2363 = arith.xori %2362, %true_0 : i1
    %2364 = arith.xori %2362, %true_0 : i1 loc(#loc157)
    %2365 = arith.xori %2362, %true_0 : i1 loc(#loc157)
    %2366 = arith.xori %2362, %true_0 : i1
    %2367 = arith.andi %2186, %true : i1
    %2368 = arith.ori %2366, %2367 : i1
    %2369 = arith.ori %2365, %2368 : i1
    %2370 = arith.ori %2364, %2369 : i1
    %2371 = arith.andi %2370, %true : i1 loc(#loc149)
    %2372 = arith.ori %2363, %2371 : i1 loc(#loc149)
    %2373 = arith.andi %2361, %2372 : i1 loc(#loc149)
    %2374 = arith.xori %2362, %true_0 : i1
    %2375 = arith.andi %2370, %true : i1
    %2376 = arith.ori %2374, %2375 : i1
    %2377 = arith.andi %2373, %2376 : i1
    %2378 = arith.xori %2362, %true_0 : i1
    %2379 = arith.xori %2362, %true_0 : i1
    %2380 = arith.ori %2379, %2370 : i1
    %2381 = acir.extract %571 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc150)
    %2382 = arith.xori %2381, %true_0 : i1 loc(#loc150)
    %2383 = arith.andi %2362, %2382 : i1 loc(#loc150)
    %2384 = arith.xori %2383, %true_0 : i1
    %2385 = arith.andi %2370, %true : i1
    %2386 = arith.ori %2384, %2385 : i1
    %2387 = arith.andi %2383, %2386 : i1
    %2388 = arith.andi %2362, %2381 : i1
    %2389 = arith.xori %2388, %true_0 : i1
    %2390 = arith.andi %2370, %true : i1
    %2391 = arith.ori %2389, %2390 : i1
    %2392 = arith.andi %2391, %true : i1
    %2393 = arith.andi %2388, %2392 : i1
    %2394 = arith.ori %2387, %2393 : i1
    %2395 = arith.andi %2380, %2394 : i1
    %2396 = arith.ori %2378, %2395 : i1
    %2397 = arith.andi %2377, %2396 : i1
    %2398 = arith.xori %2362, %true_0 : i1 loc(#loc151)
    %2399 = arith.xori %2362, %true_0 : i1
    %2400 = arith.ori %2399, %2380 : i1
    %false_78 = arith.constant {acir.source_type = "bool"} false
    %2401 = acir.extract %571 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %2402 = arith.select %2383, %false_78, %2401 : i1 loc(#loc152)
    %2403 = arith.xori %2402, %true_0 : i1 loc(#loc152)
    %2404 = arith.andi %2362, %2403 : i1 loc(#loc152)
    %2405 = arith.xori %2404, %true_0 : i1
    %2406 = arith.andi %2380, %2394 : i1
    %2407 = arith.ori %2405, %2406 : i1
    %2408 = arith.andi %2404, %2407 : i1
    %2409 = arith.andi %2362, %2402 : i1
    %2410 = arith.xori %2409, %true_0 : i1
    %2411 = arith.andi %2380, %2394 : i1
    %2412 = arith.ori %2410, %2411 : i1
    %2413 = arith.andi %2368, %true : i1 loc(#loc157)
    %2414 = arith.andi %2362, %2413 : i1 loc(#loc157)
    %2415 = arith.andi %2369, %2414 : i1 loc(#loc157)
    %2416 = arith.andi %2362, %2415 : i1 loc(#loc157)
    %2417 = arith.andi %true, %2416 : i1 loc(#loc157)
    %2418 = arith.andi %2412, %2417 : i1 loc(#loc157)
    %2419 = arith.andi %2409, %2418 : i1 loc(#loc157)
    %2420 = arith.ori %2408, %2419 : i1 loc(#loc157)
    %2421 = arith.andi %2400, %2420 : i1 loc(#loc157)
    %2422 = arith.andi %2362, %2421 : i1 loc(#loc157)
    %2423 = arith.andi %2369, %2422 : i1 loc(#loc157)
    %2424 = arith.andi %2362, %2423 : i1 loc(#loc157)
    %2425 = acir.extract %571 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
    %2426 = arith.cmpi eq, %2425, %1803 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc153)
    %2427 = arith.select %2404, %false_78, %2426 : i1 loc(#loc153)
    %2428 = arith.andi %2424, %2427 : i1 loc(#loc153)
    %2429 = arith.andi %2368, %2428 : i1 loc(#loc153)
    %2430 = arith.ori %2398, %2429 : i1 loc(#loc153)
    %2431 = arith.andi %2397, %2430 : i1 loc(#loc153)
    %2432 = arith.xori %2362, %true_0 : i1 loc(#loc154)
    %2433 = arith.xori %2362, %true_0 : i1 loc(#loc151)
    %2434 = arith.andi %2368, %2428 : i1 loc(#loc151)
    %2435 = arith.ori %2433, %2434 : i1 loc(#loc151)
    %2436 = arith.andi %2435, %true : i1 loc(#loc154)
    %2437 = arith.ori %2432, %2436 : i1 loc(#loc154)
    %2438 = arith.andi %2431, %2437 : i1 loc(#loc154)
    %2439 = arith.xori %2362, %true_0 : i1 loc(#loc155)
    %2440 = arith.ori %2439, %2435 : i1 loc(#loc155)
    %2441 = arith.andi %2438, %2440 : i1 loc(#loc155)
    %2442 = arith.xori %2182, %true_0 : i1 loc(#loc149)
    %2443 = arith.xori %2182, %true_0 : i1 loc(#loc158)
    %2444 = arith.xori %2182, %true_0 : i1 loc(#loc158)
    %2445 = arith.xori %2182, %true_0 : i1
    %2446 = arith.ori %2445, %true : i1
    %2447 = arith.ori %2444, %2446 : i1
    %2448 = arith.ori %2443, %2447 : i1
    %2449 = arith.andi %2448, %true : i1 loc(#loc149)
    %2450 = arith.ori %2442, %2449 : i1 loc(#loc149)
    %2451 = arith.andi %2441, %2450 : i1 loc(#loc149)
    %2452 = arith.xori %2182, %true_0 : i1
    %2453 = arith.andi %2448, %true : i1
    %2454 = arith.ori %2452, %2453 : i1
    %2455 = arith.andi %2451, %2454 : i1
    %2456 = arith.xori %2182, %true_0 : i1
    %2457 = arith.xori %2182, %true_0 : i1
    %2458 = arith.ori %2457, %2448 : i1
    %2459 = acir.extract %643 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc150)
    %2460 = arith.xori %2459, %true_0 : i1 loc(#loc150)
    %2461 = arith.andi %2182, %2460 : i1 loc(#loc150)
    %2462 = arith.xori %2461, %true_0 : i1
    %2463 = arith.andi %2448, %true : i1
    %2464 = arith.ori %2462, %2463 : i1
    %2465 = arith.andi %2461, %2464 : i1
    %2466 = arith.andi %2182, %2459 : i1
    %2467 = arith.xori %2466, %true_0 : i1
    %2468 = arith.andi %2448, %true : i1
    %2469 = arith.ori %2467, %2468 : i1
    %2470 = arith.andi %2469, %true : i1
    %2471 = arith.andi %2466, %2470 : i1
    %2472 = arith.ori %2465, %2471 : i1
    %2473 = arith.andi %2458, %2472 : i1
    %2474 = arith.ori %2456, %2473 : i1
    %2475 = arith.andi %2455, %2474 : i1
    %2476 = arith.xori %2182, %true_0 : i1 loc(#loc151)
    %2477 = arith.xori %2182, %true_0 : i1
    %2478 = arith.ori %2477, %2458 : i1
    %false_79 = arith.constant {acir.source_type = "bool"} false
    %2479 = acir.extract %643 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %2480 = arith.select %2461, %false_79, %2479 : i1 loc(#loc152)
    %2481 = arith.xori %2480, %true_0 : i1 loc(#loc152)
    %2482 = arith.andi %2182, %2481 : i1 loc(#loc152)
    %2483 = arith.xori %2482, %true_0 : i1
    %2484 = arith.andi %2458, %2472 : i1
    %2485 = arith.ori %2483, %2484 : i1
    %2486 = arith.andi %2482, %2485 : i1
    %2487 = arith.andi %2182, %2480 : i1
    %2488 = arith.xori %2487, %true_0 : i1
    %2489 = arith.andi %2458, %2472 : i1
    %2490 = arith.ori %2488, %2489 : i1
    %2491 = arith.andi %2446, %true : i1 loc(#loc158)
    %2492 = arith.andi %2182, %2491 : i1 loc(#loc158)
    %2493 = arith.andi %2447, %2492 : i1 loc(#loc158)
    %2494 = arith.andi %2182, %2493 : i1 loc(#loc158)
    %2495 = arith.andi %true, %2494 : i1 loc(#loc158)
    %2496 = arith.andi %2490, %2495 : i1 loc(#loc158)
    %2497 = arith.andi %2487, %2496 : i1 loc(#loc158)
    %2498 = arith.ori %2486, %2497 : i1 loc(#loc158)
    %2499 = arith.andi %2478, %2498 : i1 loc(#loc158)
    %2500 = arith.andi %2182, %2499 : i1 loc(#loc158)
    %2501 = arith.andi %2447, %2500 : i1 loc(#loc158)
    %2502 = arith.andi %2182, %2501 : i1 loc(#loc158)
    %2503 = acir.extract %643 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
    %2504 = arith.cmpi eq, %2503, %1803 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc153)
    %2505 = arith.select %2482, %false_79, %2504 : i1 loc(#loc153)
    %2506 = arith.andi %2502, %2505 : i1 loc(#loc153)
    %2507 = arith.andi %2446, %2506 : i1 loc(#loc153)
    %2508 = arith.ori %2476, %2507 : i1 loc(#loc153)
    %2509 = arith.andi %2475, %2508 : i1 loc(#loc153)
    %2510 = arith.xori %2182, %true_0 : i1 loc(#loc154)
    %2511 = arith.xori %2182, %true_0 : i1 loc(#loc151)
    %2512 = arith.andi %2446, %2506 : i1 loc(#loc151)
    %2513 = arith.ori %2511, %2512 : i1 loc(#loc151)
    %2514 = arith.andi %2513, %true : i1 loc(#loc154)
    %2515 = arith.ori %2510, %2514 : i1 loc(#loc154)
    %2516 = arith.andi %2509, %2515 : i1 loc(#loc154)
    %2517 = arith.xori %2182, %true_0 : i1 loc(#loc155)
    %2518 = arith.ori %2517, %2513 : i1 loc(#loc155)
    %2519 = arith.andi %2516, %2518 : i1 loc(#loc155)
    %2520 = arith.andi %2519, %true : i1 loc(#loc155)
    %2521 = arith.xori %2183, %true_0 : i1
    %2522 = arith.ori %2521, %2186 : i1
    %2523 = arith.xori %2192, %true_0 : i1
    %2524 = arith.ori %2523, %2196 : i1
    %2525 = arith.andi %2275, %true : i1 loc(#loc159)
    %2526 = arith.andi %2202, %2525 : i1 loc(#loc159)
    %2527 = arith.andi %2526, %2268 : i1 loc(#loc159)
    %2528 = arith.andi %2208, %2527 : i1 loc(#loc159)
    %2529 = arith.andi %2202, %2528 : i1 loc(#loc159)
    %2530 = arith.andi %2355, %true : i1 loc(#loc159)
    %2531 = arith.andi %2282, %2530 : i1 loc(#loc159)
    %2532 = arith.andi %2531, %2348 : i1 loc(#loc159)
    %2533 = arith.andi %2288, %2532 : i1 loc(#loc159)
    %2534 = arith.andi %2282, %2533 : i1 loc(#loc159)
    %2535 = arith.ori %2529, %2534 : i1 loc(#loc159)
    %2536 = arith.andi %2524, %2535 : i1 loc(#loc159)
    %2537 = arith.andi %2192, %2536 : i1 loc(#loc159)
    %2538 = arith.andi %2435, %true : i1 loc(#loc159)
    %2539 = arith.andi %2362, %2538 : i1 loc(#loc159)
    %2540 = arith.andi %2539, %2428 : i1 loc(#loc159)
    %2541 = arith.andi %2368, %2540 : i1 loc(#loc159)
    %2542 = arith.andi %2362, %2541 : i1 loc(#loc159)
    %2543 = arith.ori %2537, %2542 : i1 loc(#loc159)
    %2544 = arith.andi %2522, %2543 : i1 loc(#loc159)
    %2545 = arith.andi %2183, %2544 : i1 loc(#loc159)
    %2546 = arith.andi %2513, %true : i1 loc(#loc159)
    %2547 = arith.andi %2182, %2546 : i1 loc(#loc159)
    %2548 = arith.andi %2547, %2506 : i1 loc(#loc159)
    %2549 = arith.andi %2446, %2548 : i1 loc(#loc159)
    %2550 = arith.andi %2182, %2549 : i1 loc(#loc159)
    %2551 = arith.ori %2545, %2550 : i1 loc(#loc159)
    %2552 = arith.andi %2520, %2551 : i1 loc(#loc159)
    %2553 = arith.select %2202, %423, %497 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2554 = arith.select %2192, %2553, %571 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2555 = arith.select %2183, %2554, %643 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2556 = acir.extract %2555 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %c0_i32_80 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc160)
    %2557 = arith.cmpi ne, %2556, %c0_i32_80 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc160)
    %2558 = arith.xori %2557, %true_0 : i1 loc(#loc160)
    %2559 = arith.xori %2557, %true_0 : i1
    %2560 = arith.ori %2559, %2551 : i1
    %2561 = arith.andi %2560, %true : i1 loc(#loc161)
    %2562 = arith.ori %2558, %2561 : i1 loc(#loc161)
    %2563 = arith.andi %2552, %2562 : i1 loc(#loc161)
    %2564 = arith.xori %2557, %true_0 : i1 loc(#loc161)
    %2565 = arith.andi %2560, %true : i1 loc(#loc161)
    %2566 = arith.ori %2564, %2565 : i1 loc(#loc161)
    %2567 = arith.andi %2563, %2566 : i1 loc(#loc161)
    %2568 = arith.xori %2557, %true_0 : i1 loc(#loc161)
    %2569 = arith.andi %true, %2551 : i1 loc(#loc161)
    %2570 = arith.andi %2569, %2551 : i1 loc(#loc161)
    %2571 = arith.andi %2560, %2570 : i1 loc(#loc161)
    %2572 = arith.ori %2568, %2571 : i1 loc(#loc161)
    %2573 = arith.andi %2567, %2572 : i1 loc(#loc161)
    %2574 = arith.xori %2557, %true_0 : i1 loc(#loc162)
    %2575 = arith.andi %2560, %true : i1 loc(#loc162)
    %2576 = arith.ori %2574, %2575 : i1 loc(#loc162)
    %2577 = arith.andi %2573, %2576 : i1 loc(#loc162)
    %2578 = arith.xori %2557, %true_0 : i1 loc(#loc162)
    %2579 = arith.andi %2560, %true : i1 loc(#loc162)
    %2580 = arith.ori %2578, %2579 : i1 loc(#loc162)
    %2581 = arith.andi %2577, %2580 : i1 loc(#loc162)
    %2582 = arith.xori %2557, %true_0 : i1 loc(#loc162)
    %2583 = arith.andi %true, %2551 : i1 loc(#loc162)
    %2584 = arith.andi %2583, %2551 : i1 loc(#loc162)
    %2585 = arith.andi %2560, %2584 : i1 loc(#loc162)
    %2586 = arith.ori %2582, %2585 : i1 loc(#loc162)
    %2587 = arith.andi %2581, %2586 : i1 loc(#loc162)
    %2588 = acir.current {aspect = "value", resource = "top.commit.retired"} : i32 loc(#loc163)
    %2589 = arith.andi %2587, %true : i1 loc(#loc163)
    %2590 = arith.andi %2589, %true : i1 loc(#loc163)
    %2591 = arith.andi %2590, %2551 : i1 loc(#loc163)
    %2592 = acir.fire "top.commit.commit#7" %2591 {successors = [], txn = 7 : i64} loc(#loc163)
    %2593 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc164)
    %2594 = arith.andi %970, %390 : i1 loc(#loc79)
    %2595 = arith.select %2594, %392, %2593 : !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc79)
    %2596 = arith.ori %false, %2594 : i1 loc(#loc79)
    %2597 = arith.andi %2596, %2596 : i1 loc(#loc79)
    %2598 = acir.fire "top.audit.observe#8" %2597 {successors = [], txn = 8 : i64} loc(#loc79)
    %c0_i32_81 = arith.constant 0 : i32 loc(#loc79)
    %c1_i32_82 = arith.constant 1 : i32 loc(#loc79)
    %c0_i32_83 = arith.constant 0 : i32 loc(#loc79)
    %2599 = arith.select %971, %c1_i32_82, %c0_i32_83 : i32 loc(#loc79)
    %2600 = arith.addi %c0_i32_81, %2599 : i32 loc(#loc79)
    %2601 = arith.subi %334, %2600 : i32 loc(#loc79)
    %2602 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc79)
    %c0_i32_84 = arith.constant 0 : i32 loc(#loc79)
    %2603 = acir.extract %366[%c0_i32_84] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc79)
    %c0_i32_85 = arith.constant 0 : i32 loc(#loc79)
    %2604 = arith.cmpi eq, %2600, %c0_i32_85 : i32 loc(#loc79)
    %2605 = arith.select %2604, %2603, %2602 : !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc79)
    %2606 = acir.aggregate(%2605) : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc79)
    %2607 = arith.andi %982, %332 : i1 loc(#loc27)
    %2608 = acir.extract %323 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc165)
    %2609 = acir.extract %323 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc166)
    %2610 = acir.extract %323 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %2611 = acir.extract %2610 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc26)
    %2612 = acir.extract %323 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc167)
    %2613 = acir.extract %2612 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc167)
    %2614 = arith.addi %2611, %2613 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc167)
    %2615 = acir.aggregate(%2608, %2609, %2614) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc167)
    %c0_i32_86 = arith.constant 0 : i32 loc(#loc167)
    %2616 = arith.select %2607, %2601, %c0_i32_86 : i32 loc(#loc167)
    %2617 = acir.update %2606, %2615[%2616] {path = [unit]} : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1>, !acir.struct<"src_payloads_25ebcaa7.Completion">, i32) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc167)
    %2618 = arith.select %2607, %2617, %2606 : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc167)
    %c1_i32_87 = arith.constant 1 : i32 loc(#loc167)
    %c0_i32_88 = arith.constant 0 : i32 loc(#loc167)
    %2619 = arith.select %2607, %c1_i32_87, %c0_i32_88 : i32 loc(#loc167)
    %2620 = arith.addi %2601, %2619 : i32 loc(#loc167)
    acir.next %2618 {aspect = "storage", resource = "top.adder.out"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc167)
    acir.next %2620 {aspect = "count", resource = "top.adder.out"} : i32 loc(#loc167)
    %2621 = acir.current {aspect = "value", resource = "top.audit.last_tag"} : i32 loc(#loc167)
    %2622 = acir.extract %2595 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc168)
    %2623 = arith.select %2598, %2622, %2621 : i32 loc(#loc168)
    acir.next %2623 {aspect = "value", resource = "top.audit.last_tag"} : i32 loc(#loc168)
    %2624 = acir.current {aspect = "value", resource = "top.commit.last_tag"} : i32 loc(#loc168)
    %2625 = acir.extract %2555 {acir.id = "v11", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc169)
    %2626 = arith.select %2592, %2625, %2624 : i32 loc(#loc169)
    acir.next %2626 {aspect = "value", resource = "top.commit.last_tag"} : i32 loc(#loc169)
    %c1_i32_89 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc170)
    %2627 = arith.addi %2588, %c1_i32_89 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc170)
    %2628 = arith.select %2592, %2627, %2588 : i32 loc(#loc170)
    acir.next %2628 {aspect = "value", resource = "top.commit.retired"} : i32 loc(#loc170)
    %2629 = arith.andi %1314, %1049 : i1 loc(#loc61)
    %false_90 = arith.constant {acir.source_type = "bool"} false loc(#loc171)
    %c0_i32_91 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2630 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2631 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2632 = acir.aggregate(%false_90, %c0_i32_91, %c0_i32_91, %c0_i32_91, %2630, %2631) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2633 = arith.select %2629, %2632, %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2634 = arith.andi %970, %812 : i1 loc(#loc79)
    %2635 = acir.aggregate(%true_22, %747, %772) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc51)
    %2636 = acir.update %775, %2635 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2637 = arith.select %819, %775, %2636 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2638 = arith.select %796, %0, %2637 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2639 = arith.select %2634, %2638, %2633 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2640 = arith.andi %993, %107 : i1 loc(#loc23)
    %false_92 = arith.constant {acir.source_type = "bool"} false loc(#loc171)
    %c0_i32_93 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2641 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2642 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2643 = acir.aggregate(%false_92, %c0_i32_93, %c0_i32_93, %c0_i32_93, %2641, %2642) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2644 = arith.select %2640, %2643, %2639 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2645 = arith.andi %2178, %2146 : i1
    %true_94 = arith.constant {acir.source_type = "bool"} true loc(#loc172)
    %2646 = acir.extract %1398 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc173)
    %2647 = acir.extract %1398 {acir.id = "v15", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc174)
    %2648 = acir.aggregate(%false_60, %1456, %c0_i32_55) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc175)
    %true_95 = arith.constant {acir.source_type = "bool"} true
    %2649 = acir.extract %1564 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc176)
    %2650 = acir.aggregate(%true_95, %1456, %2649) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc176)
    %2651 = acir.extract %1603[%1430] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc106)
    %2652 = acir.aggregate(%true_95, %1456, %2651) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc106)
    %2653 = acir.aggregate(%true_95, %c0_i32_55, %c0_i32_55) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2654 = arith.select %1598, %2652, %2653 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2655 = arith.select %2089, %2650, %2654 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2656 = arith.select %2081, %2648, %2655 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2657 = acir.aggregate(%false_66, %1643, %c0_i32_61) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc175)
    %true_96 = arith.constant {acir.source_type = "bool"} true
    %2658 = acir.extract %1751 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc176)
    %2659 = acir.aggregate(%true_96, %1643, %2658) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc176)
    %2660 = acir.extract %1603[%1619] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc106)
    %2661 = acir.aggregate(%true_96, %1643, %2660) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc106)
    %2662 = acir.aggregate(%true_96, %c0_i32_61, %c0_i32_61) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2663 = arith.select %1785, %2661, %2662 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2664 = arith.select %2121, %2659, %2663 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2665 = arith.select %2113, %2657, %2664 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc177)
    %2666 = acir.aggregate(%true_94, %2646, %1798, %2647, %2656, %2665) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc177)
    %2667 = arith.select %2645, %2666, %2644 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc177)
    acir.next %2667 {aspect = "value", resource = "top.left.slot"} : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc177)
    %2668 = arith.andi %1302, %1265 : i1
    %true_97 = arith.constant {acir.source_type = "bool"} true loc(#loc178)
    %2669 = arith.select %2668, %true_97, %1264 : i1 loc(#loc178)
    %2670 = arith.andi %1378, %1331 : i1 loc(#loc78)
    %false_98 = arith.constant {acir.source_type = "bool"} false
    %2671 = arith.select %2670, %false_98, %2669 : i1
    acir.next %2671 {aspect = "value", resource = "top.multiplier.active"} : i1
    %c0_i32_99 = arith.constant 0 : i32
    %c1_i32_100 = arith.constant 1 : i32
    %c0_i32_101 = arith.constant 0 : i32
    %2672 = arith.select %1365, %c1_i32_100, %c0_i32_101 : i32
    %2673 = arith.addi %c0_i32_99, %2672 : i32
    %2674 = arith.subi %344, %2673 : i32
    %2675 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    %c0_i32_102 = arith.constant 0 : i32
    %2676 = acir.extract %342[%c0_i32_102] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    %c0_i32_103 = arith.constant 0 : i32
    %2677 = arith.cmpi eq, %2673, %c0_i32_103 : i32
    %2678 = arith.select %2677, %2676, %2675 : !acir.struct<"src_payloads_25ebcaa7.Completion">
    %2679 = acir.aggregate(%2678) : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1>
    %2680 = arith.andi %1378, %1360 : i1 loc(#loc78)
    %2681 = acir.extract %1336 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc179)
    %2682 = acir.extract %1336 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc180)
    %2683 = acir.extract %1336 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc77)
    %2684 = acir.extract %2683 {acir.id = "v25", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc77)
    %2685 = acir.extract %1336 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc181)
    %2686 = acir.extract %2685 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc181)
    %2687 = arith.muli %2684, %2686 {acir.id = "v28", acir.source_type = "u32"} : i32 loc(#loc181)
    %2688 = acir.aggregate(%2681, %2682, %2687) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc181)
    %2689 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc78)
    %2690 = arith.select %1331, %2688, %2689 : !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc78)
    %c0_i32_104 = arith.constant 0 : i32 loc(#loc78)
    %2691 = arith.select %2680, %2674, %c0_i32_104 : i32 loc(#loc78)
    %2692 = acir.update %2679, %2690[%2691] {path = [unit]} : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1>, !acir.struct<"src_payloads_25ebcaa7.Completion">, i32) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc78)
    %2693 = arith.select %2680, %2692, %2679 : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc78)
    %c1_i32_105 = arith.constant 1 : i32 loc(#loc78)
    %c0_i32_106 = arith.constant 0 : i32 loc(#loc78)
    %2694 = arith.select %2680, %c1_i32_105, %c0_i32_106 : i32 loc(#loc78)
    %2695 = arith.addi %2674, %2694 : i32 loc(#loc78)
    acir.next %2693 {aspect = "storage", resource = "top.multiplier.out"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Completion">, 1> loc(#loc78)
    acir.next %2695 {aspect = "count", resource = "top.multiplier.out"} : i32 loc(#loc78)
    %2696 = arith.andi %1302, %1265 : i1
    %c2_i32_107 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc182)
    %2697 = arith.select %2696, %c2_i32_107, %1321 : i32 loc(#loc182)
    %2698 = arith.andi %1378, %1343 : i1 loc(#loc78)
    %c1_i32_108 = arith.constant {acir.source_type = "u32"} 1 : i32
    %2699 = arith.subi %1321, %c1_i32_108 {acir.id = "v11", acir.source_type = "u32"} : i32
    %2700 = arith.select %2698, %2699, %2697 : i32
    acir.next %2700 {aspect = "value", resource = "top.multiplier.remaining"} : i32
    %2701 = arith.andi %1302, %1265 : i1
    %2702 = arith.select %2701, %1273, %1336 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.next %2702 {aspect = "value", resource = "top.multiplier.request"} : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2703 = arith.andi %2592, %2557 : i1
    %2704 = acir.extract %2555 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc183)
    %2705 = acir.extract %2555 {acir.id = "v30", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc184)
    %2706 = acir.update %1443, %2704[%2705] {acir.id = "v31", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc184)
    %2707 = arith.select %2703, %2706, %1443 : !acir.array<i32, 8> loc(#loc184)
    acir.next %2707 {aspect = "value", resource = "top.rf.committed"} : !acir.array<i32, 8> loc(#loc184)
    %2708 = arith.andi %2592, %2557 : i1
    %2709 = acir.extract %2555 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc185)
    %2710 = acir.extract %2555 {acir.id = "v17", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc186)
    %2711 = acir.update %1603, %2709[%2710] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc186)
    %2712 = arith.select %2708, %2711, %1603 : !acir.array<i32, 8> loc(#loc186)
    acir.next %2712 {aspect = "value", resource = "top.rf.registers"} : !acir.array<i32, 8> loc(#loc186)
    %2713 = arith.andi %2178, %2014 : i1
    %2714 = acir.update %1438, %1798[%2012] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc131)
    %2715 = arith.select %2713, %2714, %1438 : !acir.array<i32, 8> loc(#loc131)
    acir.next %2715 {aspect = "value", resource = "top.rf.renamed"} : !acir.array<i32, 8> loc(#loc131)
    %2716 = arith.andi %1314, %1050 : i1 loc(#loc61)
    %false_109 = arith.constant {acir.source_type = "bool"} false loc(#loc171)
    %c0_i32_110 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2717 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2718 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2719 = acir.aggregate(%false_109, %c0_i32_110, %c0_i32_110, %c0_i32_110, %2717, %2718) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2720 = arith.select %2716, %2719, %46 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2721 = arith.andi %970, %942 : i1 loc(#loc79)
    %2722 = acir.aggregate(%true_24, %877, %902) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc51)
    %2723 = acir.update %905, %2722 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2724 = arith.select %949, %905, %2723 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2725 = arith.select %926, %46, %2724 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2726 = arith.select %2721, %2725, %2720 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc51)
    %2727 = arith.andi %993, %108 : i1 loc(#loc23)
    %false_111 = arith.constant {acir.source_type = "bool"} false loc(#loc171)
    %c0_i32_112 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2728 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2729 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc171)
    %2730 = acir.aggregate(%false_111, %c0_i32_112, %c0_i32_112, %c0_i32_112, %2728, %2729) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2731 = arith.select %2727, %2730, %2726 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
    %2732 = arith.andi %2178, %2048 : i1
    %2733 = arith.select %2732, %2666, %2731 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.next %2733 {aspect = "value", resource = "top.right.slot"} : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2734 = arith.addi %1803, %c1_i32_72 {acir.id = "v38", acir.source_type = "u32"} : i32 loc(#loc187)
    %2735 = arith.select %2592, %2734, %1803 : i32 loc(#loc187)
    acir.next %2735 {aspect = "value", resource = "top.rob.head"} : i32 loc(#loc187)
    %2736 = arith.andi %970, %699 : i1 loc(#loc79)
    %true_113 = arith.constant {acir.source_type = "bool"} true
    %2737 = acir.update %643, %true_113 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2738 = acir.extract %392 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc188)
    %2739 = acir.update %2737, %2738 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc188)
    %2740 = arith.select %2736, %2739, %643 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc188)
    %2741 = arith.andi %2178, %1961 : i1
    %true_114 = arith.constant {acir.source_type = "bool"} true loc(#loc189)
    %false_115 = arith.constant {acir.source_type = "bool"} false loc(#loc190)
    %2742 = acir.extract %1398 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc191)
    %c0_i32_116 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc192)
    %2743 = acir.aggregate(%true_114, %false_115, %1798, %2742, %c0_i32_116) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2744 = arith.select %2741, %2743, %2740 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2745 = arith.andi %2592, %2182 : i1
    %false_117 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_118 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2746 = acir.aggregate(%false_117, %false_117, %c0_i32_118, %c0_i32_118, %c0_i32_118) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2747 = arith.select %2745, %2746, %2744 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.next %2747 {aspect = "value", resource = "top.rob.slot0.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2748 = arith.andi %970, %627 : i1 loc(#loc79)
    %true_119 = arith.constant {acir.source_type = "bool"} true
    %2749 = acir.update %571, %true_119 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2750 = acir.update %2749, %2738 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2751 = arith.select %2748, %2750, %571 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2752 = arith.andi %2178, %1926 : i1
    %true_120 = arith.constant {acir.source_type = "bool"} true loc(#loc189)
    %false_121 = arith.constant {acir.source_type = "bool"} false loc(#loc190)
    %c0_i32_122 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc192)
    %2753 = acir.aggregate(%true_120, %false_121, %1798, %2742, %c0_i32_122) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2754 = arith.select %2752, %2753, %2751 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2755 = arith.andi %2592, %2362 : i1
    %false_123 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_124 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2756 = acir.aggregate(%false_123, %false_123, %c0_i32_124, %c0_i32_124, %c0_i32_124) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2757 = arith.select %2755, %2756, %2754 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.next %2757 {aspect = "value", resource = "top.rob.slot1.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2758 = arith.andi %970, %553 : i1 loc(#loc79)
    %true_125 = arith.constant {acir.source_type = "bool"} true
    %2759 = acir.update %497, %true_125 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2760 = acir.update %2759, %2738 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2761 = arith.select %2758, %2760, %497 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2762 = arith.andi %2178, %1891 : i1
    %true_126 = arith.constant {acir.source_type = "bool"} true loc(#loc189)
    %false_127 = arith.constant {acir.source_type = "bool"} false loc(#loc190)
    %c0_i32_128 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc192)
    %2763 = acir.aggregate(%true_126, %false_127, %1798, %2742, %c0_i32_128) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2764 = arith.select %2762, %2763, %2761 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2765 = arith.andi %2592, %2282 : i1
    %false_129 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_130 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2766 = acir.aggregate(%false_129, %false_129, %c0_i32_130, %c0_i32_130, %c0_i32_130) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2767 = arith.select %2765, %2766, %2764 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.next %2767 {aspect = "value", resource = "top.rob.slot2.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2768 = arith.andi %970, %479 : i1 loc(#loc79)
    %true_131 = arith.constant {acir.source_type = "bool"} true
    %2769 = acir.update %423, %true_131 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2770 = acir.update %2769, %2738 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2771 = arith.select %2768, %2770, %423 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2772 = arith.andi %2178, %1854 : i1
    %true_132 = arith.constant {acir.source_type = "bool"} true loc(#loc189)
    %false_133 = arith.constant {acir.source_type = "bool"} false loc(#loc190)
    %c0_i32_134 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc192)
    %2773 = acir.aggregate(%true_132, %false_133, %1798, %2742, %c0_i32_134) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2774 = arith.select %2772, %2773, %2771 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc192)
    %2775 = arith.andi %2592, %2202 : i1
    %false_135 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_136 = arith.constant {acir.source_type = "u32"} 0 : i32
    %2776 = acir.aggregate(%false_135, %false_135, %c0_i32_136, %c0_i32_136, %c0_i32_136) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2777 = arith.select %2775, %2776, %2774 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.next %2777 {aspect = "value", resource = "top.rob.slot3.state"} : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2778 = arith.andi %2178, %1382 : i1
    %2779 = arith.addi %1798, %c1_i32_67 {acir.id = "v40", acir.source_type = "u32"} : i32 loc(#loc193)
    %2780 = arith.select %2778, %2779, %1798 : i32 loc(#loc193)
    acir.next %2780 {aspect = "value", resource = "top.rob.tail"} : i32 loc(#loc193)
    %c0_i32_137 = arith.constant 0 : i32 loc(#loc193)
    %c1_i32_138 = arith.constant 1 : i32 loc(#loc193)
    %c0_i32_139 = arith.constant 0 : i32 loc(#loc193)
    %2781 = arith.select %982, %c1_i32_138, %c0_i32_139 : i32 loc(#loc193)
    %2782 = arith.addi %c0_i32_137, %2781 : i32 loc(#loc193)
    %2783 = arith.subi %321, %2782 : i32 loc(#loc193)
    %2784 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc193)
    %c0_i32_140 = arith.constant 0 : i32 loc(#loc193)
    %2785 = acir.extract %322[%c0_i32_140] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc193)
    %c0_i32_141 = arith.constant 0 : i32 loc(#loc193)
    %2786 = arith.cmpi eq, %2782, %c0_i32_141 : i32 loc(#loc193)
    %2787 = arith.select %2786, %2785, %2784 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc193)
    %2788 = acir.aggregate(%2787) : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc193)
    %2789 = arith.andi %993, %319 : i1 loc(#loc23)
    %2790 = arith.select %108, %46, %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %c0_i32_142 = arith.constant 0 : i32 loc(#loc23)
    %2791 = arith.select %2789, %2783, %c0_i32_142 : i32 loc(#loc23)
    %2792 = acir.update %2788, %2790[%2791] {path = [unit]} : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1>, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc23)
    %2793 = arith.select %2789, %2792, %2788 : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc23)
    %c1_i32_143 = arith.constant 1 : i32 loc(#loc23)
    %c0_i32_144 = arith.constant 0 : i32 loc(#loc23)
    %2794 = arith.select %2789, %c1_i32_143, %c0_i32_144 : i32 loc(#loc23)
    %2795 = arith.addi %2783, %2794 : i32 loc(#loc23)
    acir.next %2793 {aspect = "storage", resource = "top.scheduler.add_requests"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc23)
    acir.next %2795 {aspect = "count", resource = "top.scheduler.add_requests"} : i32 loc(#loc23)
    %c0_i32_145 = arith.constant 0 : i32 loc(#loc23)
    %c1_i32_146 = arith.constant 1 : i32 loc(#loc23)
    %c0_i32_147 = arith.constant 0 : i32 loc(#loc23)
    %2796 = arith.select %1303, %c1_i32_146, %c0_i32_147 : i32 loc(#loc23)
    %2797 = arith.addi %c0_i32_145, %2796 : i32 loc(#loc23)
    %2798 = arith.subi %1263, %2797 : i32 loc(#loc23)
    %2799 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %c0_i32_148 = arith.constant 0 : i32 loc(#loc23)
    %2800 = acir.extract %1272[%c0_i32_148] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1>, i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %c0_i32_149 = arith.constant 0 : i32 loc(#loc23)
    %2801 = arith.cmpi eq, %2797, %c0_i32_149 : i32 loc(#loc23)
    %2802 = arith.select %2801, %2800, %2799 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %2803 = acir.aggregate(%2802) : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc23)
    %2804 = arith.andi %1314, %1261 : i1 loc(#loc61)
    %2805 = arith.select %1050, %46, %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc61)
    %c0_i32_150 = arith.constant 0 : i32 loc(#loc61)
    %2806 = arith.select %2804, %2798, %c0_i32_150 : i32 loc(#loc61)
    %2807 = acir.update %2803, %2805[%2806] {path = [unit]} : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1>, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc61)
    %2808 = arith.select %2804, %2807, %2803 : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc61)
    %c1_i32_151 = arith.constant 1 : i32 loc(#loc61)
    %c0_i32_152 = arith.constant 0 : i32 loc(#loc61)
    %2809 = arith.select %2804, %c1_i32_151, %c0_i32_152 : i32 loc(#loc61)
    %2810 = arith.addi %2798, %2809 : i32 loc(#loc61)
    acir.next %2808 {aspect = "storage", resource = "top.scheduler.mul_requests"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.Entry">, 1> loc(#loc61)
    acir.next %2810 {aspect = "count", resource = "top.scheduler.mul_requests"} : i32 loc(#loc61)
    %2811 = arith.andi %2178, %1382 : i1
    %c0_i32_153 = arith.constant 0 : i32
    %c1_i32_154 = arith.constant 1 : i32
    %c0_i32_155 = arith.constant 0 : i32
    %2812 = arith.select %2811, %c1_i32_154, %c0_i32_155 : i32
    %2813 = arith.addi %c0_i32_153, %2812 : i32
    %2814 = arith.subi %1399, %2813 : i32
    %2815 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c3_i32_156 = arith.constant 3 : i32
    %2816 = acir.extract %1397[%c3_i32_156] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c3_i32_157 = arith.constant 3 : i32
    %2817 = arith.cmpi eq, %2813, %c3_i32_157 : i32
    %2818 = arith.select %2817, %2816, %2815 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c2_i32_158 = arith.constant 2 : i32
    %2819 = acir.extract %1397[%c2_i32_158] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c2_i32_159 = arith.constant 2 : i32
    %2820 = arith.cmpi eq, %2813, %c2_i32_159 : i32
    %2821 = arith.select %2820, %2819, %2818 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c1_i32_160 = arith.constant 1 : i32
    %2822 = acir.extract %1397[%c1_i32_160] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c1_i32_161 = arith.constant 1 : i32
    %2823 = arith.cmpi eq, %2813, %c1_i32_161 : i32
    %2824 = arith.select %2823, %2822, %2821 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c0_i32_162 = arith.constant 0 : i32
    %2825 = acir.extract %1397[%c0_i32_162] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c0_i32_163 = arith.constant 0 : i32
    %2826 = arith.cmpi eq, %2813, %c0_i32_163 : i32
    %2827 = arith.select %2826, %2825, %2824 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %2828 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c3_i32_164 = arith.constant 3 : i32
    %2829 = acir.extract %1397[%c3_i32_164] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c2_i32_165 = arith.constant 2 : i32
    %2830 = arith.cmpi eq, %2813, %c2_i32_165 : i32
    %2831 = arith.select %2830, %2829, %2828 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c2_i32_166 = arith.constant 2 : i32
    %2832 = acir.extract %1397[%c2_i32_166] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c1_i32_167 = arith.constant 1 : i32
    %2833 = arith.cmpi eq, %2813, %c1_i32_167 : i32
    %2834 = arith.select %2833, %2832, %2831 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c1_i32_168 = arith.constant 1 : i32
    %2835 = acir.extract %1397[%c1_i32_168] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c0_i32_169 = arith.constant 0 : i32
    %2836 = arith.cmpi eq, %2813, %c0_i32_169 : i32
    %2837 = arith.select %2836, %2835, %2834 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %2838 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c3_i32_170 = arith.constant 3 : i32
    %2839 = acir.extract %1397[%c3_i32_170] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c1_i32_171 = arith.constant 1 : i32
    %2840 = arith.cmpi eq, %2813, %c1_i32_171 : i32
    %2841 = arith.select %2840, %2839, %2838 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c2_i32_172 = arith.constant 2 : i32
    %2842 = acir.extract %1397[%c2_i32_172] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c0_i32_173 = arith.constant 0 : i32
    %2843 = arith.cmpi eq, %2813, %c0_i32_173 : i32
    %2844 = arith.select %2843, %2842, %2841 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %2845 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c3_i32_174 = arith.constant 3 : i32
    %2846 = acir.extract %1397[%c3_i32_174] : (!acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %c0_i32_175 = arith.constant 0 : i32
    %2847 = arith.cmpi eq, %2813, %c0_i32_175 : i32
    %2848 = arith.select %2847, %2846, %2845 : !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %2849 = acir.aggregate(%2827, %2837, %2844, %2848) : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> !acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>
    acir.next %2849 {aspect = "storage", resource = "top.source"} : !acir.array<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, 4>
    acir.next %2814 {aspect = "count", resource = "top.source"} : i32
    %2850 = arith.andi %2178, %1382 : i1
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc194)
    %2851 = arith.cmpi ult, %1430, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc194)
    acir.effect %2850(%2851) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc84)
    %2852 = arith.andi %2178, %1382 : i1
    %c8_i32_176 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc194)
    %2853 = arith.cmpi ult, %1619, %c8_i32_176 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc194)
    acir.effect %2852(%2853) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc84)
    %2854 = arith.andi %2178, %1382 : i1
    %c8_i32_177 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc195)
    %2855 = arith.cmpi ult, %2012, %c8_i32_177 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc195)
    acir.effect %2854(%2855) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc127)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:13)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:13)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:31)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:47)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:51)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:59)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":17:16)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:17)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:16)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:32)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:20)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:55)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:75)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:17)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:9)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:20)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":17:9)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:9)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:13)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":48:17)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:48)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:9)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:27)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:21)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:14)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:13)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:12)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:33)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:50)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:13)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:13)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":131:13)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":30:17)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:12)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:20)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:40)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:29)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:40)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:29)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:29)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:51)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:59)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":23:16)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:20)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":23:9)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:13)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":80:16)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":66:19)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":84:13)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":66:9)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:9)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:9)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:12)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":82:19)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:25)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:17)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":76:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:9)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:25)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:16)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:48)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":74:13)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":49:9)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:9)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:15)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:19)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":177:17)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:27)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:21)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:14)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:16)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:16)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:20)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:9)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:20)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:13)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:20)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:13)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:9)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:13)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:28)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:44)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:13)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:39)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:16)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:23)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:20)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:30)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:15)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:27)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:21)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:14)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:13)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:20)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:20)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:9)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:9)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:13)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:13)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:13)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:9)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:9)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":185:18)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":186:13)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:9)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:20)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":14:20)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:9)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":16:9)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:9)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:20)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:13)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:15)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:27)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:21)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:14)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":150:21)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":71:17)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":74:20)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:32)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:48)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:17)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:9)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:21)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:21)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:21)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":78:9)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":190:24)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:13)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:13)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:19)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":82:17)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:27)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:38)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:66)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":83:20)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:20)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:29)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:16)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:23)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:16)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:39)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":173:20)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:18)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:27)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:38)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:66)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:21)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:35)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:23)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:35)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:23)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":151:16)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:26)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:32)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:48)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:16)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:24)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:21)

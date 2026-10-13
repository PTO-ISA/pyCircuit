module attributes {acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, exports = {}, format_version = 8 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", type = "bool"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default, name = "lhs", type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", type = "bool"}, {default = false, name = "done", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}]}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 5 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.event", source_branch = 29 : i64, target_branches = [59], to = 8 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}], schedule = [3, 1, 5, 2, 0, 4, 6, 7, 8], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.add_done : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.add_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.audit.last_tag : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.commit.last_tag : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.commit.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.event : !acir.struct<"src_payloads_25ebcaa7.Completion"> {initial = [0 : i32, 0 : i32, 0 : i32], kind = "signal"}
    acir.state @top.left.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]], kind = "cell"}
    acir.state @top.mul_done : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.mul_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.multiplier.active : i1 {initial = false, kind = "cell"}
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
    acir.state @top.source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity = 4 : i64, initial = [[1 : i32, 1 : i32, 2 : i32, 3 : i32], [0 : i32, 4 : i32, 1 : i32, 2 : i32], [0 : i32, 5 : i32, 2 : i32, 3 : i32], [0 : i32, 6 : i32, 4 : i32, 5 : i32]], kind = "queue"}
    %true_0 = arith.constant {acir.source_type = "bool"} true
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_1 = arith.constant {acir.source_type = "bool"} false
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    acir.path %true {branch = 0 : i64, frames = ["top.left:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.left:peek"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    acir.path %true {branch = 0 : i64, frames = ["top.right:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc3)
    %value_2, %ready_3 = acir.read(%true) {branch = 0 : i64, frames = ["top.right:peek"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    %0 = acir.extract %value {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc4)
    %1 = arith.xori %0, %true : i1
    %2 = arith.xori %1, %true : i1
    %3 = arith.ori %2, %ready : i1
    %4 = arith.xori %0, %true : i1
    %5 = arith.ori %4, %ready : i1
    %6 = acir.extract %value {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc5)
    %7 = acir.extract %6 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc5)
    %8 = arith.xori %1, %true : i1
    %9 = arith.ori %8, %3 : i1
    %10 = arith.ori %1, %0 : i1
    %11 = arith.xori %0, %true : i1
    %12 = arith.ori %11, %5 : i1
    %13 = arith.andi %9, %12 : i1
    %14 = arith.andi %1, %3 : i1
    %15 = arith.andi %5, %ready : i1
    %16 = arith.andi %0, %15 : i1
    %17 = arith.ori %14, %16 : i1
    %18 = arith.select %1, %false_1, %7 : i1
    %19 = arith.xori %18, %true : i1
    %20 = arith.xori %19, %true : i1
    %21 = arith.ori %20, %17 : i1
    %22 = arith.xori %18, %true : i1
    %23 = arith.ori %22, %17 : i1
    %24 = acir.extract %value {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc6)
    %25 = acir.extract %24 {acir.id = "v13", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc6)
    %26 = arith.xori %19, %true : i1
    %27 = arith.ori %26, %21 : i1
    %28 = arith.ori %19, %18 : i1
    %29 = arith.xori %18, %true : i1
    %30 = arith.ori %29, %23 : i1
    %31 = arith.andi %27, %30 : i1
    %32 = arith.andi %19, %21 : i1
    %33 = arith.andi %23, %ready : i1
    %34 = arith.andi %18, %33 : i1
    %35 = arith.ori %32, %34 : i1
    %36 = arith.select %19, %false_1, %25 : i1
    %37 = acir.extract %value_2 {acir.id = "v18", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc7)
    %38 = arith.xori %37, %true : i1
    %39 = arith.xori %38, %true : i1
    %40 = arith.ori %39, %ready_3 : i1
    %41 = arith.xori %37, %true : i1
    %42 = arith.ori %41, %ready_3 : i1
    %43 = acir.extract %value_2 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc8)
    %44 = acir.extract %43 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc8)
    %45 = arith.xori %38, %true : i1
    %46 = arith.ori %45, %40 : i1
    %47 = arith.ori %38, %37 : i1
    %48 = arith.xori %37, %true : i1
    %49 = arith.ori %48, %42 : i1
    %50 = arith.andi %46, %49 : i1
    %51 = arith.andi %38, %40 : i1
    %52 = arith.andi %42, %ready_3 : i1
    %53 = arith.andi %37, %52 : i1
    %54 = arith.ori %51, %53 : i1
    %55 = arith.select %38, %false_1, %44 : i1
    %56 = arith.xori %55, %true : i1
    %57 = arith.xori %56, %true : i1
    %58 = arith.ori %57, %54 : i1
    %59 = arith.xori %55, %true : i1
    %60 = arith.ori %59, %54 : i1
    %61 = acir.extract %value_2 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc9)
    %62 = acir.extract %61 {acir.id = "v28", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc9)
    %63 = arith.xori %56, %true : i1
    %64 = arith.ori %63, %58 : i1
    %65 = arith.ori %56, %55 : i1
    %66 = arith.xori %55, %true : i1
    %67 = arith.ori %66, %60 : i1
    %68 = arith.andi %64, %67 : i1
    %69 = arith.andi %56, %58 : i1
    %70 = arith.andi %60, %ready_3 : i1
    %71 = arith.andi %55, %70 : i1
    %72 = arith.ori %69, %71 : i1
    %73 = arith.select %56, %false_1, %62 : i1
    %74 = arith.xori %36, %true : i1
    %75 = arith.xori %74, %true : i1
    %76 = arith.ori %75, %35 : i1
    %77 = arith.xori %36, %true : i1
    %78 = arith.ori %77, %35 : i1
    %79 = acir.extract %value {acir.id = "v34", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc10)
    %80 = arith.cmpi eq, %79, %c0_i32 {acir.id = "v36", acir.source_type = "bool"} : i32 loc(#loc10)
    %81 = arith.xori %74, %true : i1
    %82 = arith.ori %81, %76 : i1
    %83 = arith.ori %74, %36 : i1
    %84 = arith.xori %36, %true : i1
    %85 = arith.ori %84, %78 : i1
    %86 = arith.andi %82, %85 : i1
    %87 = arith.andi %74, %76 : i1
    %88 = arith.andi %78, %ready : i1
    %89 = arith.andi %36, %88 : i1
    %90 = arith.ori %87, %89 : i1
    %91 = arith.select %74, %false_1, %80 : i1
    %92 = arith.xori %73, %true : i1
    %93 = arith.xori %92, %true : i1
    %94 = arith.ori %93, %72 : i1
    %95 = arith.xori %73, %true : i1
    %96 = arith.ori %95, %72 : i1
    %97 = acir.extract %value_2 {acir.id = "v42", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc11)
    %98 = arith.cmpi eq, %97, %c0_i32 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc11)
    %99 = arith.xori %92, %true : i1
    %100 = arith.ori %99, %94 : i1
    %101 = arith.ori %92, %73 : i1
    %102 = arith.xori %73, %true : i1
    %103 = arith.ori %102, %96 : i1
    %104 = arith.andi %100, %103 : i1
    %105 = arith.andi %92, %94 : i1
    %106 = arith.andi %96, %ready_3 : i1
    %107 = arith.andi %73, %106 : i1
    %108 = arith.ori %105, %107 : i1
    %109 = arith.select %92, %false_1, %98 : i1
    %110 = arith.xori %36, %true : i1
    %111 = arith.xori %110, %true : i1
    %112 = arith.ori %111, %35 : i1
    %113 = arith.xori %36, %true : i1
    %114 = arith.ori %113, %35 : i1
    %115 = acir.extract %value {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc12)
    %116 = arith.cmpi eq, %115, %c1_i32 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc12)
    %117 = arith.xori %110, %true : i1
    %118 = arith.ori %117, %112 : i1
    %119 = arith.ori %110, %36 : i1
    %120 = arith.xori %36, %true : i1
    %121 = arith.ori %120, %114 : i1
    %122 = arith.andi %118, %121 : i1
    %123 = arith.andi %110, %112 : i1
    %124 = arith.andi %114, %ready : i1
    %125 = arith.andi %36, %124 : i1
    %126 = arith.ori %123, %125 : i1
    %127 = arith.select %110, %false_1, %116 : i1
    %128 = arith.xori %73, %true : i1
    %129 = arith.xori %128, %true : i1
    %130 = arith.ori %129, %72 : i1
    %131 = arith.xori %73, %true : i1
    %132 = arith.ori %131, %72 : i1
    %133 = acir.extract %value_2 {acir.id = "v58", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc13)
    %134 = arith.cmpi eq, %133, %c1_i32 {acir.id = "v60", acir.source_type = "bool"} : i32 loc(#loc13)
    %135 = arith.xori %128, %true : i1
    %136 = arith.ori %135, %130 : i1
    %137 = arith.ori %128, %73 : i1
    %138 = arith.xori %73, %true : i1
    %139 = arith.ori %138, %132 : i1
    %140 = arith.andi %136, %139 : i1
    %141 = arith.andi %128, %130 : i1
    %142 = arith.andi %132, %ready_3 : i1
    %143 = arith.andi %73, %142 : i1
    %144 = arith.ori %141, %143 : i1
    %145 = arith.select %128, %false_1, %134 : i1
    %146 = arith.xori %91, %true : i1
    %147 = arith.xori %146, %true : i1
    %148 = arith.ori %147, %90 : i1
    %149 = arith.xori %91, %true : i1
    %150 = arith.ori %149, %90 : i1
    %151 = acir.unary %109 "not" {acir.id = "v66", acir.source_type = "bool"} : i1 -> i1 loc(#loc14)
    %152 = arith.andi %91, %151 : i1
    %153 = arith.andi %150, %108 : i1
    %154 = arith.xori %151, %true : i1
    %155 = arith.andi %91, %154 : i1
    %156 = arith.andi %150, %108 : i1
    %157 = arith.xori %155, %true : i1
    %158 = arith.ori %157, %156 : i1
    %159 = acir.extract %value {acir.id = "v69", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc15)
    %160 = acir.extract %value_2 {acir.id = "v70", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc16)
    %161 = arith.andi %ready, %ready_3 : i1 loc(#loc15)
    %162 = arith.cmpi ult, %159, %160 {acir.id = "v71", acir.source_type = "bool"} : i32 loc(#loc15)
    %163 = arith.xori %152, %true : i1
    %164 = arith.ori %163, %153 : i1
    %165 = arith.xori %155, %true : i1
    %166 = arith.ori %165, %158 : i1
    %167 = arith.ori %155, %152 : i1
    %168 = arith.xori %152, %true : i1
    %169 = arith.ori %168, %164 : i1
    %170 = arith.andi %166, %169 : i1
    %171 = arith.andi %158, %161 : i1
    %172 = arith.andi %155, %171 : i1
    %173 = arith.andi %152, %164 : i1
    %174 = arith.ori %172, %173 : i1
    %175 = arith.select %155, %162, %true_0 : i1
    %176 = arith.xori %91, %true : i1
    %177 = arith.ori %176, %150 : i1
    %178 = arith.xori %146, %true : i1
    %179 = arith.ori %178, %148 : i1
    %180 = arith.ori %146, %91 : i1
    %181 = arith.xori %91, %true : i1
    %182 = arith.ori %181, %177 : i1
    %183 = arith.andi %179, %182 : i1
    %184 = arith.andi %146, %148 : i1
    %185 = arith.andi %177, %174 : i1
    %186 = arith.andi %91, %185 : i1
    %187 = arith.ori %184, %186 : i1
    %188 = arith.select %146, %false_1, %175 : i1
    %189 = arith.xori %127, %true : i1
    %190 = arith.xori %189, %true : i1
    %191 = arith.ori %190, %126 : i1
    %192 = arith.xori %127, %true : i1
    %193 = arith.ori %192, %126 : i1
    %194 = acir.unary %145 "not" {acir.id = "v80", acir.source_type = "bool"} : i1 -> i1 loc(#loc17)
    %195 = arith.andi %127, %194 : i1
    %196 = arith.andi %193, %144 : i1
    %197 = arith.xori %194, %true : i1
    %198 = arith.andi %127, %197 : i1
    %199 = arith.andi %193, %144 : i1
    %200 = arith.xori %198, %true : i1
    %201 = arith.ori %200, %199 : i1
    %202 = acir.extract %value {acir.id = "v83", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc18)
    %203 = acir.extract %value_2 {acir.id = "v84", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc19)
    %204 = arith.andi %ready, %ready_3 : i1 loc(#loc18)
    %205 = arith.cmpi ult, %202, %203 {acir.id = "v85", acir.source_type = "bool"} : i32 loc(#loc18)
    %206 = arith.xori %195, %true : i1
    %207 = arith.ori %206, %196 : i1
    %208 = arith.xori %198, %true : i1
    %209 = arith.ori %208, %201 : i1
    %210 = arith.ori %198, %195 : i1
    %211 = arith.xori %195, %true : i1
    %212 = arith.ori %211, %207 : i1
    %213 = arith.andi %209, %212 : i1
    %214 = arith.andi %201, %204 : i1
    %215 = arith.andi %198, %214 : i1
    %216 = arith.andi %195, %207 : i1
    %217 = arith.ori %215, %216 : i1
    %218 = arith.select %198, %205, %true_0 : i1
    %219 = arith.xori %127, %true : i1
    %220 = arith.ori %219, %193 : i1
    %221 = arith.xori %189, %true : i1
    %222 = arith.ori %221, %191 : i1
    %223 = arith.ori %189, %127 : i1
    %224 = arith.xori %127, %true : i1
    %225 = arith.ori %224, %220 : i1
    %226 = arith.andi %222, %225 : i1
    %227 = arith.andi %189, %191 : i1
    %228 = arith.andi %220, %217 : i1
    %229 = arith.andi %127, %228 : i1
    %230 = arith.ori %227, %229 : i1
    %231 = arith.select %189, %false_1, %218 : i1
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc20)
    acir.path %true {branch = 1 : i64, frames = ["top.scheduler:issue_add"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc20)
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32
    %232 = arith.andi %187, %187 : i1
    %233 = arith.xori %188, %true : i1
    %234 = arith.xori %233, %true : i1
    %235 = arith.ori %234, %187 : i1
    acir.path %233 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc21)
    acir.path %233 {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc21)
    %236 = arith.xori %233, %true : i1 loc(#loc21)
    %237 = arith.ori %236, %235 : i1 loc(#loc21)
    %238 = arith.andi %233, %235 : i1 loc(#loc21)
    %239 = arith.xori %233, %true : i1 loc(#loc21)
    %240 = arith.ori %239, %237 : i1 loc(#loc21)
    %241 = arith.andi %237, %238 : i1 loc(#loc21)
    %242 = arith.andi %233, %241 : i1 loc(#loc21)
    %false_5 = arith.constant {acir.source_type = "bool"} false
    %value_6, %ready_7 = acir.read(%233) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %243 = arith.xori %233, %true : i1 loc(#loc22)
    %244 = arith.andi %240, %ready_7 : i1 loc(#loc22)
    %245 = arith.ori %243, %244 : i1 loc(#loc22)
    %246 = arith.andi %232, %245 : i1 loc(#loc22)
    %247 = acir.extract %value_6 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %248 = arith.xori %233, %true : i1
    %249 = arith.andi %240, %ready_7 : i1
    %250 = arith.ori %248, %249 : i1
    %251 = arith.andi %246, %250 : i1
    %252 = arith.andi %233, %247 : i1
    %253 = arith.andi %240, %ready_7 : i1
    %254 = arith.xori %247, %true : i1
    %255 = arith.andi %233, %254 : i1
    %256 = arith.andi %240, %ready_7 : i1
    %257 = arith.xori %255, %true : i1
    %258 = arith.ori %257, %256 : i1
    %259 = arith.xori %252, %true : i1
    %260 = arith.ori %259, %253 : i1
    %261 = acir.extract %value_6 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %262 = arith.andi %ready_7, %242 : i1 loc(#loc24)
    %263 = arith.cmpi eq, %261, %c0_i32_4 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc24)
    %264 = arith.xori %255, %true : i1
    %265 = arith.ori %264, %258 : i1
    %266 = arith.ori %255, %252 : i1
    %267 = arith.xori %252, %true : i1
    %268 = arith.ori %267, %260 : i1
    %269 = arith.andi %265, %268 : i1
    %270 = arith.andi %255, %258 : i1
    %271 = arith.andi %260, %262 : i1
    %272 = arith.andi %252, %271 : i1
    %273 = arith.ori %270, %272 : i1
    %274 = arith.select %255, %false_5, %263 : i1
    %275 = arith.xori %233, %true : i1
    %276 = arith.ori %275, %240 : i1
    %277 = arith.xori %233, %true : i1
    %278 = arith.andi %276, %273 : i1
    %279 = arith.ori %277, %278 : i1
    %280 = arith.andi %251, %279 : i1
    %281 = arith.andi %233, %274 : i1
    %282 = arith.andi %276, %273 : i1
    %283 = arith.xori %274, %true : i1
    %284 = arith.andi %233, %283 : i1
    %285 = arith.andi %276, %273 : i1
    %286 = arith.xori %284, %true : i1
    %287 = arith.ori %286, %285 : i1
    %288 = arith.xori %281, %true : i1
    %289 = arith.ori %288, %282 : i1
    %290 = acir.extract %value_6 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %291 = acir.extract %290 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %292 = arith.xori %284, %true : i1
    %293 = arith.ori %292, %287 : i1
    %294 = arith.ori %284, %281 : i1
    %295 = arith.xori %281, %true : i1
    %296 = arith.ori %295, %289 : i1
    %297 = arith.andi %293, %296 : i1
    %298 = arith.andi %284, %287 : i1
    %299 = arith.andi %289, %ready_7 : i1
    %300 = arith.andi %281, %299 : i1
    %301 = arith.ori %298, %300 : i1
    %302 = arith.select %284, %false_5, %291 : i1
    %303 = arith.xori %233, %true : i1
    %304 = arith.ori %303, %276 : i1
    %305 = arith.xori %233, %true : i1
    %306 = arith.andi %304, %301 : i1
    %307 = arith.ori %305, %306 : i1
    %308 = arith.andi %280, %307 : i1
    %309 = arith.andi %233, %302 : i1
    %310 = arith.andi %304, %301 : i1
    %311 = arith.xori %302, %true : i1
    %312 = arith.andi %233, %311 : i1
    %313 = arith.andi %304, %301 : i1
    %314 = arith.xori %312, %true : i1
    %315 = arith.ori %314, %313 : i1
    %316 = arith.xori %309, %true : i1
    %317 = arith.ori %316, %310 : i1
    %318 = acir.extract %value_6 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %319 = acir.extract %318 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %320 = arith.xori %312, %true : i1
    %321 = arith.ori %320, %315 : i1
    %322 = arith.ori %312, %309 : i1
    %323 = arith.xori %309, %true : i1
    %324 = arith.ori %323, %317 : i1
    %325 = arith.andi %321, %324 : i1
    %326 = arith.andi %312, %315 : i1
    %327 = arith.andi %317, %ready_7 : i1
    %328 = arith.andi %309, %327 : i1
    %329 = arith.ori %326, %328 : i1
    %330 = arith.select %312, %false_5, %319 : i1
    %331 = arith.xori %233, %true : i1
    %332 = arith.ori %331, %304 : i1
    %333 = arith.andi %332, %329 : i1 loc(#loc27)
    %334 = arith.andi %233, %333 : i1 loc(#loc27)
    %335 = arith.andi %237, %334 : i1 loc(#loc28)
    %336 = arith.andi %233, %335 : i1 loc(#loc28)
    %337 = arith.andi %336, %330 : i1 loc(#loc28)
    %338 = arith.xori %233, %true : i1 loc(#loc28)
    %339 = arith.andi %235, %337 : i1 loc(#loc28)
    %340 = arith.ori %338, %339 : i1 loc(#loc28)
    %341 = arith.andi %308, %340 : i1 loc(#loc28)
    %342 = arith.andi %235, %337 : i1 loc(#loc28)
    %343 = arith.xori %233, %true : i1 loc(#loc28)
    %344 = arith.ori %343, %342 : i1 loc(#loc28)
    %345 = arith.andi %233, %342 : i1 loc(#loc28)
    %c0_i32_8 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_9 = arith.constant {acir.source_type = "bool"} false loc(#loc29)
    %value_10, %ready_11 = acir.read(%233) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc30)
    %346 = arith.xori %233, %true : i1 loc(#loc30)
    %347 = arith.andi %344, %ready_11 : i1 loc(#loc30)
    %348 = arith.ori %346, %347 : i1 loc(#loc30)
    %349 = arith.andi %341, %348 : i1 loc(#loc30)
    %350 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %351 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %352 = acir.aggregate(%false_9, %c0_i32_8, %c0_i32_8, %c0_i32_8, %350, %351) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc29)
    %353 = arith.xori %233, %true : i1 loc(#loc31)
    %354 = arith.ori %353, %344 : i1 loc(#loc31)
    %355 = arith.andi %349, %354 : i1 loc(#loc31)
    %356 = arith.andi %344, %ready_11 : i1 loc(#loc32)
    %357 = arith.andi %233, %356 : i1 loc(#loc32)
    %358 = arith.andi %357, %337 : i1 loc(#loc32)
    %359 = arith.xori %188, %true : i1 loc(#loc33)
    %360 = arith.ori %359, %187 : i1 loc(#loc33)
    acir.path %188 {branch = 3 : i64, kind = "branch", txn = 0 : i64} loc(#loc34)
    acir.path %188 {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc34)
    %361 = arith.xori %188, %true : i1 loc(#loc34)
    %362 = arith.ori %361, %360 : i1 loc(#loc34)
    %363 = arith.andi %188, %360 : i1 loc(#loc34)
    %364 = arith.xori %188, %true : i1 loc(#loc34)
    %365 = arith.ori %364, %362 : i1 loc(#loc34)
    %366 = arith.andi %362, %363 : i1 loc(#loc34)
    %367 = arith.andi %188, %366 : i1 loc(#loc34)
    %false_12 = arith.constant {acir.source_type = "bool"} false
    %value_13, %ready_14 = acir.read(%188) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %368 = arith.xori %188, %true : i1 loc(#loc22)
    %369 = arith.andi %365, %ready_14 : i1 loc(#loc22)
    %370 = arith.ori %368, %369 : i1 loc(#loc22)
    %371 = arith.andi %355, %370 : i1 loc(#loc22)
    %372 = acir.extract %value_13 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %373 = arith.xori %188, %true : i1
    %374 = arith.andi %365, %ready_14 : i1
    %375 = arith.ori %373, %374 : i1
    %376 = arith.andi %371, %375 : i1
    %377 = arith.andi %188, %372 : i1
    %378 = arith.andi %365, %ready_14 : i1
    %379 = arith.xori %372, %true : i1
    %380 = arith.andi %188, %379 : i1
    %381 = arith.andi %365, %ready_14 : i1
    %382 = arith.xori %380, %true : i1
    %383 = arith.ori %382, %381 : i1
    %384 = arith.xori %377, %true : i1
    %385 = arith.ori %384, %378 : i1
    %386 = acir.extract %value_13 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %387 = arith.andi %ready_14, %367 : i1 loc(#loc24)
    %388 = arith.cmpi eq, %386, %c0_i32_4 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc24)
    %389 = arith.xori %380, %true : i1
    %390 = arith.ori %389, %383 : i1
    %391 = arith.ori %380, %377 : i1
    %392 = arith.xori %377, %true : i1
    %393 = arith.ori %392, %385 : i1
    %394 = arith.andi %390, %393 : i1
    %395 = arith.andi %380, %383 : i1
    %396 = arith.andi %385, %387 : i1
    %397 = arith.andi %377, %396 : i1
    %398 = arith.ori %395, %397 : i1
    %399 = arith.select %380, %false_12, %388 : i1
    %400 = arith.xori %188, %true : i1
    %401 = arith.ori %400, %365 : i1
    %402 = arith.xori %188, %true : i1
    %403 = arith.andi %401, %398 : i1
    %404 = arith.ori %402, %403 : i1
    %405 = arith.andi %376, %404 : i1
    %406 = arith.andi %188, %399 : i1
    %407 = arith.andi %401, %398 : i1
    %408 = arith.xori %399, %true : i1
    %409 = arith.andi %188, %408 : i1
    %410 = arith.andi %401, %398 : i1
    %411 = arith.xori %409, %true : i1
    %412 = arith.ori %411, %410 : i1
    %413 = arith.xori %406, %true : i1
    %414 = arith.ori %413, %407 : i1
    %415 = acir.extract %value_13 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %416 = acir.extract %415 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %417 = arith.xori %409, %true : i1
    %418 = arith.ori %417, %412 : i1
    %419 = arith.ori %409, %406 : i1
    %420 = arith.xori %406, %true : i1
    %421 = arith.ori %420, %414 : i1
    %422 = arith.andi %418, %421 : i1
    %423 = arith.andi %409, %412 : i1
    %424 = arith.andi %414, %ready_14 : i1
    %425 = arith.andi %406, %424 : i1
    %426 = arith.ori %423, %425 : i1
    %427 = arith.select %409, %false_12, %416 : i1
    %428 = arith.xori %188, %true : i1
    %429 = arith.ori %428, %401 : i1
    %430 = arith.xori %188, %true : i1
    %431 = arith.andi %429, %426 : i1
    %432 = arith.ori %430, %431 : i1
    %433 = arith.andi %405, %432 : i1
    %434 = arith.andi %188, %427 : i1
    %435 = arith.andi %429, %426 : i1
    %436 = arith.xori %427, %true : i1
    %437 = arith.andi %188, %436 : i1
    %438 = arith.andi %429, %426 : i1
    %439 = arith.xori %437, %true : i1
    %440 = arith.ori %439, %438 : i1
    %441 = arith.xori %434, %true : i1
    %442 = arith.ori %441, %435 : i1
    %443 = acir.extract %value_13 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %444 = acir.extract %443 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %445 = arith.xori %437, %true : i1
    %446 = arith.ori %445, %440 : i1
    %447 = arith.ori %437, %434 : i1
    %448 = arith.xori %434, %true : i1
    %449 = arith.ori %448, %442 : i1
    %450 = arith.andi %446, %449 : i1
    %451 = arith.andi %437, %440 : i1
    %452 = arith.andi %442, %ready_14 : i1
    %453 = arith.andi %434, %452 : i1
    %454 = arith.ori %451, %453 : i1
    %455 = arith.select %437, %false_12, %444 : i1
    %456 = arith.xori %188, %true : i1
    %457 = arith.ori %456, %429 : i1
    %458 = arith.andi %457, %454 : i1 loc(#loc27)
    %459 = arith.andi %188, %458 : i1 loc(#loc27)
    %460 = arith.andi %362, %459 : i1 loc(#loc28)
    %461 = arith.andi %188, %460 : i1 loc(#loc28)
    %462 = arith.andi %461, %455 : i1 loc(#loc28)
    %463 = arith.xori %188, %true : i1 loc(#loc28)
    %464 = arith.andi %360, %462 : i1 loc(#loc28)
    %465 = arith.ori %463, %464 : i1 loc(#loc28)
    %466 = arith.andi %433, %465 : i1 loc(#loc28)
    %467 = arith.andi %360, %462 : i1 loc(#loc28)
    %468 = arith.xori %188, %true : i1 loc(#loc28)
    %469 = arith.ori %468, %467 : i1 loc(#loc28)
    %470 = arith.andi %188, %467 : i1 loc(#loc28)
    %c0_i32_15 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_16 = arith.constant {acir.source_type = "bool"} false loc(#loc29)
    %value_17, %ready_18 = acir.read(%188) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc30)
    %471 = arith.xori %188, %true : i1 loc(#loc30)
    %472 = arith.andi %469, %ready_18 : i1 loc(#loc30)
    %473 = arith.ori %471, %472 : i1 loc(#loc30)
    %474 = arith.andi %466, %473 : i1 loc(#loc30)
    %475 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %476 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %477 = acir.aggregate(%false_16, %c0_i32_15, %c0_i32_15, %c0_i32_15, %475, %476) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc29)
    %478 = arith.xori %188, %true : i1 loc(#loc31)
    %479 = arith.ori %478, %469 : i1 loc(#loc31)
    %480 = arith.andi %474, %479 : i1 loc(#loc31)
    %481 = arith.andi %469, %ready_18 : i1 loc(#loc32)
    %482 = arith.andi %188, %481 : i1 loc(#loc32)
    %483 = arith.andi %482, %462 : i1 loc(#loc32)
    %484 = arith.andi %235, %358 : i1 loc(#loc35)
    %485 = arith.andi %233, %484 : i1 loc(#loc35)
    %486 = arith.andi %360, %483 : i1 loc(#loc35)
    %487 = arith.andi %188, %486 : i1 loc(#loc35)
    %488 = arith.ori %485, %487 : i1 loc(#loc35)
    %489 = arith.select %233, %value_10, %value_17 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    %490 = arith.xori %488, %true : i1 loc(#loc35)
    acir.path %488 {branch = 4 : i64, kind = "branch", txn = 0 : i64} loc(#loc35)
    %value_19, %ready_20 = acir.read(%488) {branch = 4 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.add_requests", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc35)
    %491 = arith.xori %488, %true : i1 loc(#loc35)
    %492 = arith.ori %491, %ready_20 : i1 loc(#loc35)
    %493 = arith.andi %480, %492 : i1 loc(#loc35)
    %494 = arith.xori %value_19, %true : i1 loc(#loc35)
    %495 = arith.xori %488, %true : i1 loc(#loc35)
    %496 = arith.ori %495, %494 : i1 loc(#loc35)
    %497 = arith.andi %493, %496 : i1 loc(#loc35)
    %498 = acir.fire "top.scheduler.issue_add#0" %497 {successors = [], txn = 0 : i64} loc(#loc35)
    %499 = arith.andi %498, %233 : i1 loc(#loc35)
    acir.effect %499(%352) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc31)
    %500 = arith.andi %498, %188 : i1 loc(#loc35)
    acir.effect %500(%477) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc31)
    %501 = arith.andi %498, %488 : i1 loc(#loc35)
    acir.effect %501(%489) {branch = 4 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.add_requests", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc35)
    acir.path %true {branch = 5 : i64, kind = "branch", txn = 1 : i64} loc(#loc36)
    acir.path %true {branch = 5 : i64, frames = ["top.scheduler:issue_mul"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc36)
    %c1_i32_21 = arith.constant {acir.source_type = "u32"} 1 : i32
    %502 = arith.andi %230, %230 : i1
    %503 = arith.xori %231, %true : i1
    %504 = arith.xori %503, %true : i1
    %505 = arith.ori %504, %230 : i1
    acir.path %503 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc37)
    acir.path %503 {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc37)
    %506 = arith.xori %503, %true : i1 loc(#loc37)
    %507 = arith.ori %506, %505 : i1 loc(#loc37)
    %508 = arith.andi %503, %505 : i1 loc(#loc37)
    %509 = arith.xori %503, %true : i1 loc(#loc37)
    %510 = arith.ori %509, %507 : i1 loc(#loc37)
    %511 = arith.andi %507, %508 : i1 loc(#loc37)
    %512 = arith.andi %503, %511 : i1 loc(#loc37)
    %false_22 = arith.constant {acir.source_type = "bool"} false
    %value_23, %ready_24 = acir.read(%503) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %513 = arith.xori %503, %true : i1 loc(#loc22)
    %514 = arith.andi %510, %ready_24 : i1 loc(#loc22)
    %515 = arith.ori %513, %514 : i1 loc(#loc22)
    %516 = arith.andi %502, %515 : i1 loc(#loc22)
    %517 = acir.extract %value_23 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %518 = arith.xori %503, %true : i1
    %519 = arith.andi %510, %ready_24 : i1
    %520 = arith.ori %518, %519 : i1
    %521 = arith.andi %516, %520 : i1
    %522 = arith.andi %503, %517 : i1
    %523 = arith.andi %510, %ready_24 : i1
    %524 = arith.xori %517, %true : i1
    %525 = arith.andi %503, %524 : i1
    %526 = arith.andi %510, %ready_24 : i1
    %527 = arith.xori %525, %true : i1
    %528 = arith.ori %527, %526 : i1
    %529 = arith.xori %522, %true : i1
    %530 = arith.ori %529, %523 : i1
    %531 = acir.extract %value_23 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %532 = arith.andi %ready_24, %512 : i1 loc(#loc24)
    %533 = arith.cmpi eq, %531, %c1_i32_21 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc24)
    %534 = arith.xori %525, %true : i1
    %535 = arith.ori %534, %528 : i1
    %536 = arith.ori %525, %522 : i1
    %537 = arith.xori %522, %true : i1
    %538 = arith.ori %537, %530 : i1
    %539 = arith.andi %535, %538 : i1
    %540 = arith.andi %525, %528 : i1
    %541 = arith.andi %530, %532 : i1
    %542 = arith.andi %522, %541 : i1
    %543 = arith.ori %540, %542 : i1
    %544 = arith.select %525, %false_22, %533 : i1
    %545 = arith.xori %503, %true : i1
    %546 = arith.ori %545, %510 : i1
    %547 = arith.xori %503, %true : i1
    %548 = arith.andi %546, %543 : i1
    %549 = arith.ori %547, %548 : i1
    %550 = arith.andi %521, %549 : i1
    %551 = arith.andi %503, %544 : i1
    %552 = arith.andi %546, %543 : i1
    %553 = arith.xori %544, %true : i1
    %554 = arith.andi %503, %553 : i1
    %555 = arith.andi %546, %543 : i1
    %556 = arith.xori %554, %true : i1
    %557 = arith.ori %556, %555 : i1
    %558 = arith.xori %551, %true : i1
    %559 = arith.ori %558, %552 : i1
    %560 = acir.extract %value_23 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %561 = acir.extract %560 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %562 = arith.xori %554, %true : i1
    %563 = arith.ori %562, %557 : i1
    %564 = arith.ori %554, %551 : i1
    %565 = arith.xori %551, %true : i1
    %566 = arith.ori %565, %559 : i1
    %567 = arith.andi %563, %566 : i1
    %568 = arith.andi %554, %557 : i1
    %569 = arith.andi %559, %ready_24 : i1
    %570 = arith.andi %551, %569 : i1
    %571 = arith.ori %568, %570 : i1
    %572 = arith.select %554, %false_22, %561 : i1
    %573 = arith.xori %503, %true : i1
    %574 = arith.ori %573, %546 : i1
    %575 = arith.xori %503, %true : i1
    %576 = arith.andi %574, %571 : i1
    %577 = arith.ori %575, %576 : i1
    %578 = arith.andi %550, %577 : i1
    %579 = arith.andi %503, %572 : i1
    %580 = arith.andi %574, %571 : i1
    %581 = arith.xori %572, %true : i1
    %582 = arith.andi %503, %581 : i1
    %583 = arith.andi %574, %571 : i1
    %584 = arith.xori %582, %true : i1
    %585 = arith.ori %584, %583 : i1
    %586 = arith.xori %579, %true : i1
    %587 = arith.ori %586, %580 : i1
    %588 = acir.extract %value_23 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %589 = acir.extract %588 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %590 = arith.xori %582, %true : i1
    %591 = arith.ori %590, %585 : i1
    %592 = arith.ori %582, %579 : i1
    %593 = arith.xori %579, %true : i1
    %594 = arith.ori %593, %587 : i1
    %595 = arith.andi %591, %594 : i1
    %596 = arith.andi %582, %585 : i1
    %597 = arith.andi %587, %ready_24 : i1
    %598 = arith.andi %579, %597 : i1
    %599 = arith.ori %596, %598 : i1
    %600 = arith.select %582, %false_22, %589 : i1
    %601 = arith.xori %503, %true : i1
    %602 = arith.ori %601, %574 : i1
    %603 = arith.andi %602, %599 : i1 loc(#loc27)
    %604 = arith.andi %503, %603 : i1 loc(#loc27)
    %605 = arith.andi %507, %604 : i1 loc(#loc28)
    %606 = arith.andi %503, %605 : i1 loc(#loc28)
    %607 = arith.andi %606, %600 : i1 loc(#loc28)
    %608 = arith.xori %503, %true : i1 loc(#loc28)
    %609 = arith.andi %505, %607 : i1 loc(#loc28)
    %610 = arith.ori %608, %609 : i1 loc(#loc28)
    %611 = arith.andi %578, %610 : i1 loc(#loc28)
    %612 = arith.andi %505, %607 : i1 loc(#loc28)
    %613 = arith.xori %503, %true : i1 loc(#loc28)
    %614 = arith.ori %613, %612 : i1 loc(#loc28)
    %615 = arith.andi %503, %612 : i1 loc(#loc28)
    %c0_i32_25 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_26 = arith.constant {acir.source_type = "bool"} false loc(#loc29)
    %value_27, %ready_28 = acir.read(%503) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc30)
    %616 = arith.xori %503, %true : i1 loc(#loc30)
    %617 = arith.andi %614, %ready_28 : i1 loc(#loc30)
    %618 = arith.ori %616, %617 : i1 loc(#loc30)
    %619 = arith.andi %611, %618 : i1 loc(#loc30)
    %620 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %621 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %622 = acir.aggregate(%false_26, %c0_i32_25, %c0_i32_25, %c0_i32_25, %620, %621) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc29)
    %623 = arith.xori %503, %true : i1 loc(#loc31)
    %624 = arith.ori %623, %614 : i1 loc(#loc31)
    %625 = arith.andi %619, %624 : i1 loc(#loc31)
    %626 = arith.andi %614, %ready_28 : i1 loc(#loc32)
    %627 = arith.andi %503, %626 : i1 loc(#loc32)
    %628 = arith.andi %627, %607 : i1 loc(#loc32)
    %629 = arith.xori %231, %true : i1 loc(#loc38)
    %630 = arith.ori %629, %230 : i1 loc(#loc38)
    acir.path %231 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc39)
    acir.path %231 {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc39)
    %631 = arith.xori %231, %true : i1 loc(#loc39)
    %632 = arith.ori %631, %630 : i1 loc(#loc39)
    %633 = arith.andi %231, %630 : i1 loc(#loc39)
    %634 = arith.xori %231, %true : i1 loc(#loc39)
    %635 = arith.ori %634, %632 : i1 loc(#loc39)
    %636 = arith.andi %632, %633 : i1 loc(#loc39)
    %637 = arith.andi %231, %636 : i1 loc(#loc39)
    %false_29 = arith.constant {acir.source_type = "bool"} false
    %value_30, %ready_31 = acir.read(%231) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %638 = arith.xori %231, %true : i1 loc(#loc22)
    %639 = arith.andi %635, %ready_31 : i1 loc(#loc22)
    %640 = arith.ori %638, %639 : i1 loc(#loc22)
    %641 = arith.andi %625, %640 : i1 loc(#loc22)
    %642 = acir.extract %value_30 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %643 = arith.xori %231, %true : i1
    %644 = arith.andi %635, %ready_31 : i1
    %645 = arith.ori %643, %644 : i1
    %646 = arith.andi %641, %645 : i1
    %647 = arith.andi %231, %642 : i1
    %648 = arith.andi %635, %ready_31 : i1
    %649 = arith.xori %642, %true : i1
    %650 = arith.andi %231, %649 : i1
    %651 = arith.andi %635, %ready_31 : i1
    %652 = arith.xori %650, %true : i1
    %653 = arith.ori %652, %651 : i1
    %654 = arith.xori %647, %true : i1
    %655 = arith.ori %654, %648 : i1
    %656 = acir.extract %value_30 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %657 = arith.andi %ready_31, %637 : i1 loc(#loc24)
    %658 = arith.cmpi eq, %656, %c1_i32_21 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc24)
    %659 = arith.xori %650, %true : i1
    %660 = arith.ori %659, %653 : i1
    %661 = arith.ori %650, %647 : i1
    %662 = arith.xori %647, %true : i1
    %663 = arith.ori %662, %655 : i1
    %664 = arith.andi %660, %663 : i1
    %665 = arith.andi %650, %653 : i1
    %666 = arith.andi %655, %657 : i1
    %667 = arith.andi %647, %666 : i1
    %668 = arith.ori %665, %667 : i1
    %669 = arith.select %650, %false_29, %658 : i1
    %670 = arith.xori %231, %true : i1
    %671 = arith.ori %670, %635 : i1
    %672 = arith.xori %231, %true : i1
    %673 = arith.andi %671, %668 : i1
    %674 = arith.ori %672, %673 : i1
    %675 = arith.andi %646, %674 : i1
    %676 = arith.andi %231, %669 : i1
    %677 = arith.andi %671, %668 : i1
    %678 = arith.xori %669, %true : i1
    %679 = arith.andi %231, %678 : i1
    %680 = arith.andi %671, %668 : i1
    %681 = arith.xori %679, %true : i1
    %682 = arith.ori %681, %680 : i1
    %683 = arith.xori %676, %true : i1
    %684 = arith.ori %683, %677 : i1
    %685 = acir.extract %value_30 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %686 = acir.extract %685 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %687 = arith.xori %679, %true : i1
    %688 = arith.ori %687, %682 : i1
    %689 = arith.ori %679, %676 : i1
    %690 = arith.xori %676, %true : i1
    %691 = arith.ori %690, %684 : i1
    %692 = arith.andi %688, %691 : i1
    %693 = arith.andi %679, %682 : i1
    %694 = arith.andi %684, %ready_31 : i1
    %695 = arith.andi %676, %694 : i1
    %696 = arith.ori %693, %695 : i1
    %697 = arith.select %679, %false_29, %686 : i1
    %698 = arith.xori %231, %true : i1
    %699 = arith.ori %698, %671 : i1
    %700 = arith.xori %231, %true : i1
    %701 = arith.andi %699, %696 : i1
    %702 = arith.ori %700, %701 : i1
    %703 = arith.andi %675, %702 : i1
    %704 = arith.andi %231, %697 : i1
    %705 = arith.andi %699, %696 : i1
    %706 = arith.xori %697, %true : i1
    %707 = arith.andi %231, %706 : i1
    %708 = arith.andi %699, %696 : i1
    %709 = arith.xori %707, %true : i1
    %710 = arith.ori %709, %708 : i1
    %711 = arith.xori %704, %true : i1
    %712 = arith.ori %711, %705 : i1
    %713 = acir.extract %value_30 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %714 = acir.extract %713 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %715 = arith.xori %707, %true : i1
    %716 = arith.ori %715, %710 : i1
    %717 = arith.ori %707, %704 : i1
    %718 = arith.xori %704, %true : i1
    %719 = arith.ori %718, %712 : i1
    %720 = arith.andi %716, %719 : i1
    %721 = arith.andi %707, %710 : i1
    %722 = arith.andi %712, %ready_31 : i1
    %723 = arith.andi %704, %722 : i1
    %724 = arith.ori %721, %723 : i1
    %725 = arith.select %707, %false_29, %714 : i1
    %726 = arith.xori %231, %true : i1
    %727 = arith.ori %726, %699 : i1
    %728 = arith.andi %727, %724 : i1 loc(#loc27)
    %729 = arith.andi %231, %728 : i1 loc(#loc27)
    %730 = arith.andi %632, %729 : i1 loc(#loc28)
    %731 = arith.andi %231, %730 : i1 loc(#loc28)
    %732 = arith.andi %731, %725 : i1 loc(#loc28)
    %733 = arith.xori %231, %true : i1 loc(#loc28)
    %734 = arith.andi %630, %732 : i1 loc(#loc28)
    %735 = arith.ori %733, %734 : i1 loc(#loc28)
    %736 = arith.andi %703, %735 : i1 loc(#loc28)
    %737 = arith.andi %630, %732 : i1 loc(#loc28)
    %738 = arith.xori %231, %true : i1 loc(#loc28)
    %739 = arith.ori %738, %737 : i1 loc(#loc28)
    %740 = arith.andi %231, %737 : i1 loc(#loc28)
    %c0_i32_32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_33 = arith.constant {acir.source_type = "bool"} false loc(#loc29)
    %value_34, %ready_35 = acir.read(%231) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc30)
    %741 = arith.xori %231, %true : i1 loc(#loc30)
    %742 = arith.andi %739, %ready_35 : i1 loc(#loc30)
    %743 = arith.ori %741, %742 : i1 loc(#loc30)
    %744 = arith.andi %736, %743 : i1 loc(#loc30)
    %745 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %746 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %747 = acir.aggregate(%false_33, %c0_i32_32, %c0_i32_32, %c0_i32_32, %745, %746) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc29)
    %748 = arith.xori %231, %true : i1 loc(#loc31)
    %749 = arith.ori %748, %739 : i1 loc(#loc31)
    %750 = arith.andi %744, %749 : i1 loc(#loc31)
    %751 = arith.andi %739, %ready_35 : i1 loc(#loc32)
    %752 = arith.andi %231, %751 : i1 loc(#loc32)
    %753 = arith.andi %752, %732 : i1 loc(#loc32)
    %754 = arith.andi %505, %628 : i1 loc(#loc40)
    %755 = arith.andi %503, %754 : i1 loc(#loc40)
    %756 = arith.andi %630, %753 : i1 loc(#loc40)
    %757 = arith.andi %231, %756 : i1 loc(#loc40)
    %758 = arith.ori %755, %757 : i1 loc(#loc40)
    %759 = arith.select %503, %value_27, %value_34 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc40)
    %760 = arith.xori %758, %true : i1 loc(#loc40)
    acir.path %758 {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc40)
    %value_36, %ready_37 = acir.read(%758) {branch = 8 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mul_requests", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc40)
    %761 = arith.xori %758, %true : i1 loc(#loc40)
    %762 = arith.ori %761, %ready_37 : i1 loc(#loc40)
    %763 = arith.andi %750, %762 : i1 loc(#loc40)
    %764 = arith.xori %value_36, %true : i1 loc(#loc40)
    %765 = arith.xori %758, %true : i1 loc(#loc40)
    %766 = arith.ori %765, %764 : i1 loc(#loc40)
    %767 = arith.andi %763, %766 : i1 loc(#loc40)
    %768 = acir.fire "top.scheduler.issue_mul#1" %767 {successors = [], txn = 1 : i64} loc(#loc40)
    %769 = arith.andi %768, %503 : i1 loc(#loc40)
    acir.effect %769(%622) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc31)
    %770 = arith.andi %768, %231 : i1 loc(#loc40)
    acir.effect %770(%747) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc31)
    %771 = arith.andi %768, %758 : i1 loc(#loc40)
    acir.effect %771(%759) {branch = 8 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mul_requests", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc40)
    acir.path %true {branch = 9 : i64, kind = "branch", txn = 2 : i64} loc(#loc41)
    acir.path %true {branch = 9 : i64, frames = ["top.adder:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc41)
    %value_38, %ready_39 = acir.read(%true) {branch = 9 : i64, frames = ["top.adder:execute"], method = "peek", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc42)
    %772 = arith.andi %ready_39, %ready_39 : i1 loc(#loc42)
    %value_40, %ready_41 = acir.read(%true) {branch = 9 : i64, frames = ["top.adder:execute"], method = "empty", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc42)
    %773 = arith.andi %772, %ready_41 : i1 loc(#loc42)
    %774 = arith.xori %value_40, %true : i1 loc(#loc42)
    %775 = arith.andi %773, %774 : i1 loc(#loc42)
    %776 = acir.extract %value_38 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc43)
    %777 = acir.extract %value_38 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc44)
    %778 = acir.extract %value_38 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc45)
    %779 = acir.extract %778 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc45)
    %780 = acir.extract %value_38 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc46)
    %781 = acir.extract %780 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc46)
    %782 = arith.andi %ready_39, %ready_39 : i1 loc(#loc45)
    %783 = arith.addi %779, %781 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc45)
    %784 = arith.andi %ready_39, %ready_39 : i1 loc(#loc47)
    %785 = arith.andi %784, %782 : i1 loc(#loc47)
    %786 = acir.aggregate(%776, %777, %783) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc47)
    %787 = arith.xori %785, %true : i1 loc(#loc48)
    acir.path %785 {branch = 10 : i64, kind = "branch", txn = 2 : i64} loc(#loc48)
    %value_42, %ready_43 = acir.read(%785) {branch = 10 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.add_done", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc48)
    %788 = arith.xori %785, %true : i1 loc(#loc48)
    %789 = arith.ori %788, %ready_43 : i1 loc(#loc48)
    %790 = arith.andi %775, %789 : i1 loc(#loc48)
    %791 = arith.xori %value_42, %true : i1 loc(#loc48)
    %792 = arith.xori %785, %true : i1 loc(#loc48)
    %793 = arith.ori %792, %791 : i1 loc(#loc48)
    %794 = arith.andi %790, %793 : i1 loc(#loc48)
    %795 = acir.fire "top.adder.execute#2" %794 {successors = [0], txn = 2 : i64} loc(#loc48)
    acir.effect %795() {branch = 9 : i64, frames = ["top.adder:execute"], method = "pop", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> () loc(#loc42)
    %796 = arith.andi %795, %785 : i1 loc(#loc48)
    acir.effect %796(%786) {branch = 10 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.add_done", txn = 2 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> () loc(#loc48)
    %value_44, %ready_45 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.multiplier.active", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc49)
    %797 = arith.xori %value_44, %true : i1
    %798 = arith.xori %797, %true : i1
    %799 = arith.ori %798, %ready_45 : i1
    %800 = arith.andi %797, %799 : i1 loc(#loc50)
    acir.path %797 {branch = 11 : i64, kind = "branch", txn = 3 : i64} loc(#loc50)
    acir.path %797 {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc50)
    %801 = arith.xori %797, %true : i1 loc(#loc50)
    %802 = arith.ori %801, %799 : i1 loc(#loc50)
    %803 = arith.andi %797, %799 : i1 loc(#loc50)
    %true_46 = arith.constant {acir.source_type = "bool"} true loc(#loc51)
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc52)
    %value_47, %ready_48 = acir.read(%797) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "peek", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc53)
    %804 = arith.xori %797, %true : i1 loc(#loc53)
    %805 = arith.andi %802, %ready_48 : i1 loc(#loc53)
    %806 = arith.ori %804, %805 : i1 loc(#loc53)
    %807 = arith.andi %800, %806 : i1 loc(#loc53)
    %808 = arith.xori %797, %true : i1 loc(#loc53)
    %809 = arith.andi %802, %ready_48 : i1 loc(#loc53)
    %810 = arith.ori %808, %809 : i1 loc(#loc53)
    %811 = arith.andi %807, %810 : i1 loc(#loc53)
    %value_49, %ready_50 = acir.read(%797) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "empty", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc53)
    %812 = arith.xori %797, %true : i1 loc(#loc53)
    %813 = arith.andi %802, %ready_50 : i1 loc(#loc53)
    %814 = arith.ori %812, %813 : i1 loc(#loc53)
    %815 = arith.andi %811, %814 : i1 loc(#loc53)
    %816 = arith.xori %value_49, %true : i1 loc(#loc53)
    %817 = arith.xori %797, %true : i1 loc(#loc53)
    %818 = arith.andi %802, %816 : i1 loc(#loc53)
    %819 = arith.ori %817, %818 : i1 loc(#loc53)
    %820 = arith.andi %815, %819 : i1 loc(#loc53)
    %821 = arith.xori %797, %true : i1 loc(#loc54)
    %822 = arith.andi %802, %ready_48 : i1 loc(#loc54)
    %823 = arith.ori %821, %822 : i1 loc(#loc54)
    %824 = arith.andi %820, %823 : i1 loc(#loc54)
    %825 = arith.xori %797, %true : i1 loc(#loc55)
    %826 = arith.ori %825, %802 : i1 loc(#loc55)
    %827 = arith.andi %824, %826 : i1 loc(#loc55)
    %828 = arith.xori %797, %true : i1 loc(#loc56)
    %829 = arith.ori %828, %802 : i1 loc(#loc56)
    %830 = arith.andi %827, %829 : i1 loc(#loc56)
    %831 = arith.andi %797, %802 : i1
    %832 = acir.fire "top.multiplier.accept#3" %830 {successors = [1], txn = 3 : i64}
    %833 = arith.andi %832, %797 : i1
    acir.effect %833() {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "pop", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> () loc(#loc53)
    %834 = arith.andi %832, %797 : i1
    acir.effect %834(%value_47) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.request", txn = 3 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc54)
    %835 = arith.andi %832, %797 : i1
    acir.effect %835(%c2_i32) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 3 : i64} : (i1, i32) -> () loc(#loc55)
    %836 = arith.andi %832, %797 : i1
    acir.effect %836(%true_46) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 3 : i64} : (i1, i1) -> () loc(#loc56)
    %837 = arith.xori %value_44, %true : i1
    %838 = arith.ori %837, %ready_45 : i1
    %839 = arith.andi %value_44, %838 : i1 loc(#loc57)
    acir.path %value_44 {branch = 12 : i64, kind = "branch", txn = 4 : i64} loc(#loc57)
    acir.path %value_44 {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc57)
    %840 = arith.xori %value_44, %true : i1 loc(#loc57)
    %841 = arith.ori %840, %838 : i1 loc(#loc57)
    %false_51 = arith.constant {acir.source_type = "bool"} false
    %c1_i32_52 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32_53 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc58)
    %value_54, %ready_55 = acir.read(%value_44) {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc59)
    %842 = arith.xori %value_44, %true : i1 loc(#loc59)
    %843 = arith.andi %841, %ready_55 : i1 loc(#loc59)
    %844 = arith.ori %842, %843 : i1 loc(#loc59)
    %845 = arith.andi %839, %844 : i1 loc(#loc59)
    %846 = arith.cmpi ne, %value_54, %c0_i32_53 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc59)
    %847 = arith.xori %value_44, %true : i1
    %848 = arith.andi %841, %ready_55 : i1
    %849 = arith.ori %847, %848 : i1
    %850 = arith.andi %845, %849 : i1
    %851 = arith.andi %value_44, %846 : i1
    %852 = arith.andi %841, %ready_55 : i1
    %853 = arith.xori %846, %true : i1
    %854 = arith.andi %value_44, %853 : i1
    %855 = arith.andi %841, %ready_55 : i1
    %856 = arith.xori %854, %true : i1
    %857 = arith.ori %856, %855 : i1
    acir.path %854 {branch = 13 : i64, kind = "branch", txn = 4 : i64} loc(#loc60)
    %value_56, %ready_57 = acir.read(%854) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.request", txn = 4 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc60)
    %858 = arith.xori %854, %true : i1 loc(#loc60)
    %859 = arith.andi %857, %ready_57 : i1 loc(#loc60)
    %860 = arith.ori %858, %859 : i1 loc(#loc60)
    %861 = arith.andi %850, %860 : i1 loc(#loc60)
    %862 = arith.xori %854, %true : i1 loc(#loc61)
    %863 = arith.ori %862, %857 : i1 loc(#loc61)
    %864 = arith.andi %861, %863 : i1 loc(#loc61)
    %865 = acir.extract %value_56 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc62)
    %866 = acir.extract %value_56 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc63)
    %867 = acir.extract %value_56 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc64)
    %868 = acir.extract %867 {acir.id = "v25", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc64)
    %869 = acir.extract %value_56 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc65)
    %870 = acir.extract %869 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc65)
    %871 = arith.andi %ready_57, %ready_57 : i1 loc(#loc64)
    %872 = arith.muli %868, %870 {acir.id = "v28", acir.source_type = "u32"} : i32 loc(#loc64)
    %873 = arith.andi %ready_57, %ready_57 : i1 loc(#loc66)
    %874 = arith.andi %873, %871 : i1 loc(#loc66)
    %875 = acir.aggregate(%865, %866, %872) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc66)
    %876 = arith.xori %851, %true : i1 loc(#loc67)
    %877 = arith.ori %876, %852 : i1 loc(#loc67)
    acir.path %851 {branch = 14 : i64, kind = "branch", txn = 4 : i64} loc(#loc68)
    %value_58, %ready_59 = acir.read(%851) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc68)
    %878 = arith.xori %851, %true : i1 loc(#loc68)
    %879 = arith.andi %877, %ready_59 : i1 loc(#loc68)
    %880 = arith.ori %878, %879 : i1 loc(#loc68)
    %881 = arith.andi %864, %880 : i1 loc(#loc68)
    %882 = arith.subi %value_58, %c1_i32_52 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc68)
    %883 = arith.xori %851, %true : i1 loc(#loc69)
    %884 = arith.andi %877, %ready_59 : i1 loc(#loc69)
    %885 = arith.ori %883, %884 : i1 loc(#loc69)
    %886 = arith.andi %881, %885 : i1 loc(#loc69)
    %887 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc70)
    %888 = arith.andi %857, %874 : i1 loc(#loc70)
    %889 = arith.andi %854, %888 : i1 loc(#loc70)
    %890 = arith.select %854, %875, %887 : !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc70)
    %891 = arith.andi %value_44, %889 : i1 loc(#loc70)
    %892 = arith.xori %891, %true : i1 loc(#loc70)
    %893 = arith.ori %892, %838 : i1 loc(#loc70)
    %894 = arith.andi %886, %893 : i1 loc(#loc70)
    acir.path %891 {branch = 15 : i64, kind = "branch", txn = 4 : i64} loc(#loc70)
    %value_60, %ready_61 = acir.read(%891) {branch = 15 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mul_done", txn = 4 : i64} : (i1) -> (i1, i1) loc(#loc70)
    %895 = arith.xori %891, %true : i1 loc(#loc70)
    %896 = arith.andi %838, %ready_61 : i1 loc(#loc70)
    %897 = arith.ori %895, %896 : i1 loc(#loc70)
    %898 = arith.andi %894, %897 : i1 loc(#loc70)
    %899 = arith.xori %value_60, %true : i1 loc(#loc70)
    %900 = arith.xori %891, %true : i1 loc(#loc70)
    %901 = arith.andi %838, %899 : i1 loc(#loc70)
    %902 = arith.ori %900, %901 : i1 loc(#loc70)
    %903 = arith.andi %898, %902 : i1 loc(#loc70)
    %904 = acir.fire "top.multiplier.advance#4" %903 {successors = [], txn = 4 : i64} loc(#loc70)
    %905 = arith.andi %904, %854 : i1 loc(#loc70)
    acir.effect %905(%false_51) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 4 : i64} : (i1, i1) -> () loc(#loc61)
    %906 = arith.andi %904, %851 : i1 loc(#loc70)
    acir.effect %906(%882) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1, i32) -> () loc(#loc69)
    %907 = arith.andi %904, %891 : i1 loc(#loc70)
    acir.effect %907(%890) {branch = 15 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mul_done", txn = 4 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> () loc(#loc70)
    %908 = arith.andi %797, %799 : i1 loc(#loc71)
    %909 = arith.andi %value_44, %838 : i1 loc(#loc71)
    %910 = arith.ori %908, %909 : i1 loc(#loc71)
    %value_62, %ready_63 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.add_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc72)
    %911 = acir.unary %value_62 "not" {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1 loc(#loc73)
    acir.path %true {branch = 16 : i64, kind = "branch", txn = 5 : i64} loc(#loc74)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc74)
    %912 = arith.andi %ready_63, %ready_63 : i1
    %913 = arith.xori %911, %true : i1
    %914 = arith.xori %913, %true : i1
    %915 = arith.ori %914, %ready_63 : i1
    acir.path %913 {branch = 17 : i64, kind = "branch", txn = 5 : i64} loc(#loc75)
    %value_64, %ready_65 = acir.read(%913) {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc75)
    %916 = arith.xori %913, %true : i1 loc(#loc75)
    %917 = arith.andi %915, %ready_65 : i1 loc(#loc75)
    %918 = arith.ori %916, %917 : i1 loc(#loc75)
    %919 = arith.andi %912, %918 : i1 loc(#loc75)
    %920 = arith.xori %913, %true : i1 loc(#loc75)
    %921 = arith.andi %915, %ready_65 : i1 loc(#loc75)
    %922 = arith.ori %920, %921 : i1 loc(#loc75)
    %923 = arith.andi %919, %922 : i1 loc(#loc75)
    %value_66, %ready_67 = acir.read(%913) {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "empty", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> (i1, i1) loc(#loc75)
    %924 = arith.xori %913, %true : i1 loc(#loc75)
    %925 = arith.andi %915, %ready_67 : i1 loc(#loc75)
    %926 = arith.ori %924, %925 : i1 loc(#loc75)
    %927 = arith.andi %923, %926 : i1 loc(#loc75)
    %928 = arith.xori %value_66, %true : i1 loc(#loc75)
    %929 = arith.xori %913, %true : i1 loc(#loc75)
    %930 = arith.andi %915, %928 : i1 loc(#loc75)
    %931 = arith.ori %929, %930 : i1 loc(#loc75)
    %932 = arith.andi %927, %931 : i1 loc(#loc75)
    %933 = arith.xori %911, %true : i1
    %934 = arith.ori %933, %ready_63 : i1
    acir.path %911 {branch = 18 : i64, kind = "branch", txn = 5 : i64} loc(#loc76)
    %value_68, %ready_69 = acir.read(%911) {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc76)
    %935 = arith.xori %911, %true : i1 loc(#loc76)
    %936 = arith.andi %934, %ready_69 : i1 loc(#loc76)
    %937 = arith.ori %935, %936 : i1 loc(#loc76)
    %938 = arith.andi %932, %937 : i1 loc(#loc76)
    %939 = arith.xori %911, %true : i1 loc(#loc76)
    %940 = arith.andi %934, %ready_69 : i1 loc(#loc76)
    %941 = arith.ori %939, %940 : i1 loc(#loc76)
    %942 = arith.andi %938, %941 : i1 loc(#loc76)
    %value_70, %ready_71 = acir.read(%911) {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "empty", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> (i1, i1) loc(#loc76)
    %943 = arith.xori %911, %true : i1 loc(#loc76)
    %944 = arith.andi %934, %ready_71 : i1 loc(#loc76)
    %945 = arith.ori %943, %944 : i1 loc(#loc76)
    %946 = arith.andi %942, %945 : i1 loc(#loc76)
    %947 = arith.xori %value_70, %true : i1 loc(#loc76)
    %948 = arith.xori %911, %true : i1 loc(#loc76)
    %949 = arith.andi %934, %947 : i1 loc(#loc76)
    %950 = arith.ori %948, %949 : i1 loc(#loc76)
    %951 = arith.andi %946, %950 : i1 loc(#loc76)
    %952 = arith.xori %913, %true : i1
    %953 = arith.ori %952, %915 : i1
    %954 = arith.ori %913, %911 : i1
    %955 = arith.xori %911, %true : i1
    %956 = arith.ori %955, %934 : i1
    %957 = arith.andi %953, %956 : i1
    %958 = arith.andi %915, %ready_65 : i1
    %959 = arith.andi %913, %958 : i1
    %960 = arith.andi %934, %ready_69 : i1
    %961 = arith.andi %911, %960 : i1
    %962 = arith.ori %959, %961 : i1
    %963 = arith.select %913, %value_64, %value_68 : !acir.struct<"src_payloads_25ebcaa7.Completion">
    %964 = acir.extract %963 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc77)
    %965 = acir.extract %963 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc78)
    %966 = arith.andi %962, %962 : i1 loc(#loc79)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.rob:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc79)
    %c2_i32_72 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_73 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc80)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc81)
    %c1_i32_74 = arith.constant {acir.source_type = "u32"} 1 : i32
    %967 = arith.subi %964, %c1_i32_74 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc82)
    %968 = arith.andi %967, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc82)
    %969 = arith.cmpi eq, %968, %c0_i32_73 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc83)
    %970 = arith.andi %951, %962 : i1
    %971 = arith.xori %969, %true : i1
    %972 = arith.xori %971, %true : i1
    %973 = arith.ori %972, %962 : i1
    %974 = arith.cmpi eq, %968, %c1_i32_74 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc84)
    %975 = arith.xori %971, %true : i1
    %976 = arith.andi %973, %962 : i1
    %977 = arith.ori %975, %976 : i1
    %978 = arith.andi %970, %977 : i1
    %979 = arith.andi %971, %974 : i1
    %980 = arith.andi %973, %962 : i1
    %981 = arith.xori %974, %true : i1
    %982 = arith.andi %971, %981 : i1
    %983 = arith.andi %973, %962 : i1
    %984 = arith.xori %982, %true : i1
    %985 = arith.ori %984, %983 : i1
    %986 = arith.cmpi eq, %968, %c2_i32_72 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc85)
    %987 = arith.xori %982, %true : i1
    %988 = arith.andi %985, %962 : i1
    %989 = arith.ori %987, %988 : i1
    %990 = arith.andi %978, %989 : i1
    %991 = arith.andi %982, %986 : i1
    %992 = arith.andi %985, %962 : i1
    %993 = arith.xori %986, %true : i1
    %994 = arith.andi %982, %993 : i1
    %995 = arith.andi %985, %962 : i1
    %996 = arith.xori %994, %true : i1
    %997 = arith.ori %996, %995 : i1
    %998 = arith.andi %962, %962 : i1 loc(#loc86)
    acir.path %994 {branch = 19 : i64, kind = "branch", txn = 5 : i64} loc(#loc86)
    acir.path %994 {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc86)
    %999 = arith.xori %994, %true : i1 loc(#loc86)
    %1000 = arith.ori %999, %997 : i1 loc(#loc86)
    %1001 = arith.andi %997, %962 : i1 loc(#loc86)
    %1002 = arith.andi %994, %1001 : i1 loc(#loc86)
    %1003 = arith.andi %997, %962 : i1 loc(#loc86)
    %1004 = arith.andi %994, %1003 : i1 loc(#loc86)
    %true_75 = arith.constant {acir.source_type = "bool"} true
    %false_76 = arith.constant {acir.source_type = "bool"} false
    %value_77, %ready_78 = acir.read(%994) {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc87)
    %1005 = arith.xori %994, %true : i1 loc(#loc87)
    %1006 = arith.andi %1000, %ready_78 : i1 loc(#loc87)
    %1007 = arith.ori %1005, %1006 : i1 loc(#loc87)
    %1008 = arith.andi %990, %1007 : i1 loc(#loc87)
    %1009 = acir.extract %value_77 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc88)
    %1010 = arith.xori %994, %true : i1
    %1011 = arith.andi %1000, %ready_78 : i1
    %1012 = arith.ori %1010, %1011 : i1
    %1013 = arith.andi %1008, %1012 : i1
    %1014 = arith.andi %994, %1009 : i1
    %1015 = arith.andi %1000, %ready_78 : i1
    %1016 = arith.xori %1009, %true : i1
    %1017 = arith.andi %994, %1016 : i1
    %1018 = arith.andi %1000, %ready_78 : i1
    %1019 = arith.xori %1017, %true : i1
    %1020 = arith.ori %1019, %1018 : i1
    %1021 = arith.xori %1014, %true : i1
    %1022 = arith.ori %1021, %1015 : i1
    %1023 = acir.extract %value_77 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc89)
    %1024 = acir.unary %1023 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc90)
    %1025 = arith.xori %1017, %true : i1
    %1026 = arith.ori %1025, %1020 : i1
    %1027 = arith.ori %1017, %1014 : i1
    %1028 = arith.xori %1014, %true : i1
    %1029 = arith.ori %1028, %1022 : i1
    %1030 = arith.andi %1026, %1029 : i1
    %1031 = arith.andi %1017, %1020 : i1
    %1032 = arith.andi %1022, %ready_78 : i1
    %1033 = arith.andi %1014, %1032 : i1
    %1034 = arith.ori %1031, %1033 : i1
    %1035 = arith.select %1017, %false_76, %1024 : i1
    %1036 = arith.xori %994, %true : i1
    %1037 = arith.ori %1036, %1000 : i1
    %1038 = arith.xori %994, %true : i1
    %1039 = arith.andi %1037, %1034 : i1
    %1040 = arith.ori %1038, %1039 : i1
    %1041 = arith.andi %1013, %1040 : i1
    %1042 = arith.andi %994, %1035 : i1
    %1043 = arith.andi %1037, %1034 : i1
    %1044 = arith.xori %1035, %true : i1
    %1045 = arith.andi %994, %1044 : i1
    %1046 = arith.andi %1037, %1034 : i1
    %1047 = arith.xori %1045, %true : i1
    %1048 = arith.ori %1047, %1046 : i1
    %1049 = arith.xori %1042, %true : i1
    %1050 = arith.ori %1049, %1043 : i1
    %1051 = acir.extract %value_77 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc91)
    %1052 = arith.andi %ready_78, %1002 : i1 loc(#loc91)
    %1053 = arith.cmpi eq, %1051, %964 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc91)
    %1054 = arith.xori %1045, %true : i1
    %1055 = arith.ori %1054, %1048 : i1
    %1056 = arith.ori %1045, %1042 : i1
    %1057 = arith.xori %1042, %true : i1
    %1058 = arith.ori %1057, %1050 : i1
    %1059 = arith.andi %1055, %1058 : i1
    %1060 = arith.andi %1045, %1048 : i1
    %1061 = arith.andi %1050, %1052 : i1
    %1062 = arith.andi %1042, %1061 : i1
    %1063 = arith.ori %1060, %1062 : i1
    %1064 = arith.select %1045, %false_76, %1053 : i1
    %1065 = arith.xori %994, %true : i1
    %1066 = arith.ori %1065, %1037 : i1
    %1067 = arith.xori %994, %true : i1
    %1068 = arith.andi %1066, %1063 : i1
    %1069 = arith.ori %1067, %1068 : i1
    %1070 = arith.andi %1041, %1069 : i1
    %1071 = arith.andi %994, %1064 : i1
    %1072 = arith.andi %1066, %1063 : i1
    %1073 = arith.xori %1064, %true : i1
    %1074 = arith.andi %994, %1073 : i1
    %1075 = arith.andi %1066, %1063 : i1
    %1076 = arith.xori %1071, %true : i1
    %1077 = arith.ori %1076, %1072 : i1
    %1078 = acir.update %value_77, %true_75 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc92)
    %1079 = arith.andi %ready_78, %1004 : i1 loc(#loc93)
    %1080 = acir.update %1078, %965 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc93)
    %1081 = arith.xori %1071, %true : i1 loc(#loc94)
    %1082 = arith.andi %1077, %1079 : i1 loc(#loc94)
    %1083 = arith.ori %1081, %1082 : i1 loc(#loc94)
    %1084 = arith.andi %1070, %1083 : i1 loc(#loc94)
    acir.path %1071 {branch = 20 : i64, kind = "branch", txn = 5 : i64} loc(#loc94)
    %1085 = arith.xori %1074, %true : i1
    %1086 = arith.ori %1085, %1075 : i1
    %1087 = arith.ori %1074, %1071 : i1
    %1088 = arith.xori %1071, %true : i1
    %1089 = arith.ori %1088, %1077 : i1
    %1090 = arith.andi %1086, %1089 : i1
    %1091 = arith.andi %994, %1066 : i1
    %1092 = arith.xori %991, %true : i1
    %1093 = arith.ori %1092, %992 : i1
    %1094 = arith.andi %962, %962 : i1 loc(#loc95)
    acir.path %991 {branch = 21 : i64, kind = "branch", txn = 5 : i64} loc(#loc95)
    acir.path %991 {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc95)
    %1095 = arith.xori %991, %true : i1 loc(#loc95)
    %1096 = arith.ori %1095, %1093 : i1 loc(#loc95)
    %1097 = arith.andi %1093, %962 : i1 loc(#loc95)
    %1098 = arith.andi %991, %1097 : i1 loc(#loc95)
    %1099 = arith.andi %1093, %962 : i1 loc(#loc95)
    %1100 = arith.andi %991, %1099 : i1 loc(#loc95)
    %true_79 = arith.constant {acir.source_type = "bool"} true
    %false_80 = arith.constant {acir.source_type = "bool"} false
    %value_81, %ready_82 = acir.read(%991) {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc87)
    %1101 = arith.xori %991, %true : i1 loc(#loc87)
    %1102 = arith.andi %1096, %ready_82 : i1 loc(#loc87)
    %1103 = arith.ori %1101, %1102 : i1 loc(#loc87)
    %1104 = arith.andi %1084, %1103 : i1 loc(#loc87)
    %1105 = acir.extract %value_81 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc88)
    %1106 = arith.xori %991, %true : i1
    %1107 = arith.andi %1096, %ready_82 : i1
    %1108 = arith.ori %1106, %1107 : i1
    %1109 = arith.andi %1104, %1108 : i1
    %1110 = arith.andi %991, %1105 : i1
    %1111 = arith.andi %1096, %ready_82 : i1
    %1112 = arith.xori %1105, %true : i1
    %1113 = arith.andi %991, %1112 : i1
    %1114 = arith.andi %1096, %ready_82 : i1
    %1115 = arith.xori %1113, %true : i1
    %1116 = arith.ori %1115, %1114 : i1
    %1117 = arith.xori %1110, %true : i1
    %1118 = arith.ori %1117, %1111 : i1
    %1119 = acir.extract %value_81 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc89)
    %1120 = acir.unary %1119 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc90)
    %1121 = arith.xori %1113, %true : i1
    %1122 = arith.ori %1121, %1116 : i1
    %1123 = arith.ori %1113, %1110 : i1
    %1124 = arith.xori %1110, %true : i1
    %1125 = arith.ori %1124, %1118 : i1
    %1126 = arith.andi %1122, %1125 : i1
    %1127 = arith.andi %1113, %1116 : i1
    %1128 = arith.andi %1118, %ready_82 : i1
    %1129 = arith.andi %1110, %1128 : i1
    %1130 = arith.ori %1127, %1129 : i1
    %1131 = arith.select %1113, %false_80, %1120 : i1
    %1132 = arith.xori %991, %true : i1
    %1133 = arith.ori %1132, %1096 : i1
    %1134 = arith.xori %991, %true : i1
    %1135 = arith.andi %1133, %1130 : i1
    %1136 = arith.ori %1134, %1135 : i1
    %1137 = arith.andi %1109, %1136 : i1
    %1138 = arith.andi %991, %1131 : i1
    %1139 = arith.andi %1133, %1130 : i1
    %1140 = arith.xori %1131, %true : i1
    %1141 = arith.andi %991, %1140 : i1
    %1142 = arith.andi %1133, %1130 : i1
    %1143 = arith.xori %1141, %true : i1
    %1144 = arith.ori %1143, %1142 : i1
    %1145 = arith.xori %1138, %true : i1
    %1146 = arith.ori %1145, %1139 : i1
    %1147 = acir.extract %value_81 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc91)
    %1148 = arith.andi %ready_82, %1098 : i1 loc(#loc91)
    %1149 = arith.cmpi eq, %1147, %964 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc91)
    %1150 = arith.xori %1141, %true : i1
    %1151 = arith.ori %1150, %1144 : i1
    %1152 = arith.ori %1141, %1138 : i1
    %1153 = arith.xori %1138, %true : i1
    %1154 = arith.ori %1153, %1146 : i1
    %1155 = arith.andi %1151, %1154 : i1
    %1156 = arith.andi %1141, %1144 : i1
    %1157 = arith.andi %1146, %1148 : i1
    %1158 = arith.andi %1138, %1157 : i1
    %1159 = arith.ori %1156, %1158 : i1
    %1160 = arith.select %1141, %false_80, %1149 : i1
    %1161 = arith.xori %991, %true : i1
    %1162 = arith.ori %1161, %1133 : i1
    %1163 = arith.xori %991, %true : i1
    %1164 = arith.andi %1162, %1159 : i1
    %1165 = arith.ori %1163, %1164 : i1
    %1166 = arith.andi %1137, %1165 : i1
    %1167 = arith.andi %991, %1160 : i1
    %1168 = arith.andi %1162, %1159 : i1
    %1169 = arith.xori %1160, %true : i1
    %1170 = arith.andi %991, %1169 : i1
    %1171 = arith.andi %1162, %1159 : i1
    %1172 = arith.xori %1167, %true : i1
    %1173 = arith.ori %1172, %1168 : i1
    %1174 = acir.update %value_81, %true_79 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc92)
    %1175 = arith.andi %ready_82, %1100 : i1 loc(#loc93)
    %1176 = acir.update %1174, %965 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc93)
    %1177 = arith.xori %1167, %true : i1 loc(#loc94)
    %1178 = arith.andi %1173, %1175 : i1 loc(#loc94)
    %1179 = arith.ori %1177, %1178 : i1 loc(#loc94)
    %1180 = arith.andi %1166, %1179 : i1 loc(#loc94)
    acir.path %1167 {branch = 22 : i64, kind = "branch", txn = 5 : i64} loc(#loc94)
    %1181 = arith.xori %1170, %true : i1
    %1182 = arith.ori %1181, %1171 : i1
    %1183 = arith.ori %1170, %1167 : i1
    %1184 = arith.xori %1167, %true : i1
    %1185 = arith.ori %1184, %1173 : i1
    %1186 = arith.andi %1182, %1185 : i1
    %1187 = arith.andi %991, %1162 : i1
    %1188 = arith.xori %994, %true : i1
    %1189 = arith.ori %1188, %997 : i1
    %1190 = arith.ori %994, %991 : i1
    %1191 = arith.xori %991, %true : i1
    %1192 = arith.ori %1191, %1093 : i1
    %1193 = arith.andi %1189, %1192 : i1
    %1194 = arith.xori %979, %true : i1
    %1195 = arith.ori %1194, %980 : i1
    %1196 = arith.andi %962, %962 : i1 loc(#loc96)
    acir.path %979 {branch = 23 : i64, kind = "branch", txn = 5 : i64} loc(#loc96)
    acir.path %979 {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc96)
    %1197 = arith.xori %979, %true : i1 loc(#loc96)
    %1198 = arith.ori %1197, %1195 : i1 loc(#loc96)
    %1199 = arith.andi %1195, %962 : i1 loc(#loc96)
    %1200 = arith.andi %979, %1199 : i1 loc(#loc96)
    %1201 = arith.andi %1195, %962 : i1 loc(#loc96)
    %1202 = arith.andi %979, %1201 : i1 loc(#loc96)
    %true_83 = arith.constant {acir.source_type = "bool"} true
    %false_84 = arith.constant {acir.source_type = "bool"} false
    %value_85, %ready_86 = acir.read(%979) {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc87)
    %1203 = arith.xori %979, %true : i1 loc(#loc87)
    %1204 = arith.andi %1198, %ready_86 : i1 loc(#loc87)
    %1205 = arith.ori %1203, %1204 : i1 loc(#loc87)
    %1206 = arith.andi %1180, %1205 : i1 loc(#loc87)
    %1207 = acir.extract %value_85 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc88)
    %1208 = arith.xori %979, %true : i1
    %1209 = arith.andi %1198, %ready_86 : i1
    %1210 = arith.ori %1208, %1209 : i1
    %1211 = arith.andi %1206, %1210 : i1
    %1212 = arith.andi %979, %1207 : i1
    %1213 = arith.andi %1198, %ready_86 : i1
    %1214 = arith.xori %1207, %true : i1
    %1215 = arith.andi %979, %1214 : i1
    %1216 = arith.andi %1198, %ready_86 : i1
    %1217 = arith.xori %1215, %true : i1
    %1218 = arith.ori %1217, %1216 : i1
    %1219 = arith.xori %1212, %true : i1
    %1220 = arith.ori %1219, %1213 : i1
    %1221 = acir.extract %value_85 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc89)
    %1222 = acir.unary %1221 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc90)
    %1223 = arith.xori %1215, %true : i1
    %1224 = arith.ori %1223, %1218 : i1
    %1225 = arith.ori %1215, %1212 : i1
    %1226 = arith.xori %1212, %true : i1
    %1227 = arith.ori %1226, %1220 : i1
    %1228 = arith.andi %1224, %1227 : i1
    %1229 = arith.andi %1215, %1218 : i1
    %1230 = arith.andi %1220, %ready_86 : i1
    %1231 = arith.andi %1212, %1230 : i1
    %1232 = arith.ori %1229, %1231 : i1
    %1233 = arith.select %1215, %false_84, %1222 : i1
    %1234 = arith.xori %979, %true : i1
    %1235 = arith.ori %1234, %1198 : i1
    %1236 = arith.xori %979, %true : i1
    %1237 = arith.andi %1235, %1232 : i1
    %1238 = arith.ori %1236, %1237 : i1
    %1239 = arith.andi %1211, %1238 : i1
    %1240 = arith.andi %979, %1233 : i1
    %1241 = arith.andi %1235, %1232 : i1
    %1242 = arith.xori %1233, %true : i1
    %1243 = arith.andi %979, %1242 : i1
    %1244 = arith.andi %1235, %1232 : i1
    %1245 = arith.xori %1243, %true : i1
    %1246 = arith.ori %1245, %1244 : i1
    %1247 = arith.xori %1240, %true : i1
    %1248 = arith.ori %1247, %1241 : i1
    %1249 = acir.extract %value_85 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc91)
    %1250 = arith.andi %ready_86, %1200 : i1 loc(#loc91)
    %1251 = arith.cmpi eq, %1249, %964 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc91)
    %1252 = arith.xori %1243, %true : i1
    %1253 = arith.ori %1252, %1246 : i1
    %1254 = arith.ori %1243, %1240 : i1
    %1255 = arith.xori %1240, %true : i1
    %1256 = arith.ori %1255, %1248 : i1
    %1257 = arith.andi %1253, %1256 : i1
    %1258 = arith.andi %1243, %1246 : i1
    %1259 = arith.andi %1248, %1250 : i1
    %1260 = arith.andi %1240, %1259 : i1
    %1261 = arith.ori %1258, %1260 : i1
    %1262 = arith.select %1243, %false_84, %1251 : i1
    %1263 = arith.xori %979, %true : i1
    %1264 = arith.ori %1263, %1235 : i1
    %1265 = arith.xori %979, %true : i1
    %1266 = arith.andi %1264, %1261 : i1
    %1267 = arith.ori %1265, %1266 : i1
    %1268 = arith.andi %1239, %1267 : i1
    %1269 = arith.andi %979, %1262 : i1
    %1270 = arith.andi %1264, %1261 : i1
    %1271 = arith.xori %1262, %true : i1
    %1272 = arith.andi %979, %1271 : i1
    %1273 = arith.andi %1264, %1261 : i1
    %1274 = arith.xori %1269, %true : i1
    %1275 = arith.ori %1274, %1270 : i1
    %1276 = acir.update %value_85, %true_83 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc92)
    %1277 = arith.andi %ready_86, %1202 : i1 loc(#loc93)
    %1278 = acir.update %1276, %965 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc93)
    %1279 = arith.xori %1269, %true : i1 loc(#loc94)
    %1280 = arith.andi %1275, %1277 : i1 loc(#loc94)
    %1281 = arith.ori %1279, %1280 : i1 loc(#loc94)
    %1282 = arith.andi %1268, %1281 : i1 loc(#loc94)
    acir.path %1269 {branch = 24 : i64, kind = "branch", txn = 5 : i64} loc(#loc94)
    %1283 = arith.xori %1272, %true : i1
    %1284 = arith.ori %1283, %1273 : i1
    %1285 = arith.ori %1272, %1269 : i1
    %1286 = arith.xori %1269, %true : i1
    %1287 = arith.ori %1286, %1275 : i1
    %1288 = arith.andi %1284, %1287 : i1
    %1289 = arith.andi %979, %1264 : i1
    %1290 = arith.xori %982, %true : i1
    %1291 = arith.ori %1290, %985 : i1
    %1292 = arith.ori %982, %979 : i1
    %1293 = arith.xori %979, %true : i1
    %1294 = arith.ori %1293, %1195 : i1
    %1295 = arith.andi %1291, %1294 : i1
    %1296 = arith.xori %969, %true : i1
    %1297 = arith.ori %1296, %962 : i1
    %1298 = arith.andi %962, %962 : i1 loc(#loc97)
    acir.path %969 {branch = 25 : i64, kind = "branch", txn = 5 : i64} loc(#loc97)
    acir.path %969 {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc97)
    %1299 = arith.xori %969, %true : i1 loc(#loc97)
    %1300 = arith.ori %1299, %1297 : i1 loc(#loc97)
    %1301 = arith.andi %1297, %962 : i1 loc(#loc97)
    %1302 = arith.andi %969, %1301 : i1 loc(#loc97)
    %1303 = arith.andi %1297, %962 : i1 loc(#loc97)
    %1304 = arith.andi %969, %1303 : i1 loc(#loc97)
    %true_87 = arith.constant {acir.source_type = "bool"} true
    %false_88 = arith.constant {acir.source_type = "bool"} false
    %value_89, %ready_90 = acir.read(%969) {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc87)
    %1305 = arith.xori %969, %true : i1 loc(#loc87)
    %1306 = arith.andi %1300, %ready_90 : i1 loc(#loc87)
    %1307 = arith.ori %1305, %1306 : i1 loc(#loc87)
    %1308 = arith.andi %1282, %1307 : i1 loc(#loc87)
    %1309 = acir.extract %value_89 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc88)
    %1310 = arith.xori %969, %true : i1
    %1311 = arith.andi %1300, %ready_90 : i1
    %1312 = arith.ori %1310, %1311 : i1
    %1313 = arith.andi %1308, %1312 : i1
    %1314 = arith.andi %969, %1309 : i1
    %1315 = arith.andi %1300, %ready_90 : i1
    %1316 = arith.xori %1309, %true : i1
    %1317 = arith.andi %969, %1316 : i1
    %1318 = arith.andi %1300, %ready_90 : i1
    %1319 = arith.xori %1317, %true : i1
    %1320 = arith.ori %1319, %1318 : i1
    %1321 = arith.xori %1314, %true : i1
    %1322 = arith.ori %1321, %1315 : i1
    %1323 = acir.extract %value_89 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc89)
    %1324 = acir.unary %1323 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc90)
    %1325 = arith.xori %1317, %true : i1
    %1326 = arith.ori %1325, %1320 : i1
    %1327 = arith.ori %1317, %1314 : i1
    %1328 = arith.xori %1314, %true : i1
    %1329 = arith.ori %1328, %1322 : i1
    %1330 = arith.andi %1326, %1329 : i1
    %1331 = arith.andi %1317, %1320 : i1
    %1332 = arith.andi %1322, %ready_90 : i1
    %1333 = arith.andi %1314, %1332 : i1
    %1334 = arith.ori %1331, %1333 : i1
    %1335 = arith.select %1317, %false_88, %1324 : i1
    %1336 = arith.xori %969, %true : i1
    %1337 = arith.ori %1336, %1300 : i1
    %1338 = arith.xori %969, %true : i1
    %1339 = arith.andi %1337, %1334 : i1
    %1340 = arith.ori %1338, %1339 : i1
    %1341 = arith.andi %1313, %1340 : i1
    %1342 = arith.andi %969, %1335 : i1
    %1343 = arith.andi %1337, %1334 : i1
    %1344 = arith.xori %1335, %true : i1
    %1345 = arith.andi %969, %1344 : i1
    %1346 = arith.andi %1337, %1334 : i1
    %1347 = arith.xori %1345, %true : i1
    %1348 = arith.ori %1347, %1346 : i1
    %1349 = arith.xori %1342, %true : i1
    %1350 = arith.ori %1349, %1343 : i1
    %1351 = acir.extract %value_89 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc91)
    %1352 = arith.andi %ready_90, %1302 : i1 loc(#loc91)
    %1353 = arith.cmpi eq, %1351, %964 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc91)
    %1354 = arith.xori %1345, %true : i1
    %1355 = arith.ori %1354, %1348 : i1
    %1356 = arith.ori %1345, %1342 : i1
    %1357 = arith.xori %1342, %true : i1
    %1358 = arith.ori %1357, %1350 : i1
    %1359 = arith.andi %1355, %1358 : i1
    %1360 = arith.andi %1345, %1348 : i1
    %1361 = arith.andi %1350, %1352 : i1
    %1362 = arith.andi %1342, %1361 : i1
    %1363 = arith.ori %1360, %1362 : i1
    %1364 = arith.select %1345, %false_88, %1353 : i1
    %1365 = arith.xori %969, %true : i1
    %1366 = arith.ori %1365, %1337 : i1
    %1367 = arith.xori %969, %true : i1
    %1368 = arith.andi %1366, %1363 : i1
    %1369 = arith.ori %1367, %1368 : i1
    %1370 = arith.andi %1341, %1369 : i1
    %1371 = arith.andi %969, %1364 : i1
    %1372 = arith.andi %1366, %1363 : i1
    %1373 = arith.xori %1364, %true : i1
    %1374 = arith.andi %969, %1373 : i1
    %1375 = arith.andi %1366, %1363 : i1
    %1376 = arith.xori %1371, %true : i1
    %1377 = arith.ori %1376, %1372 : i1
    %1378 = acir.update %value_89, %true_87 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc92)
    %1379 = arith.andi %ready_90, %1304 : i1 loc(#loc93)
    %1380 = acir.update %1378, %965 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc93)
    %1381 = arith.xori %1371, %true : i1 loc(#loc94)
    %1382 = arith.andi %1377, %1379 : i1 loc(#loc94)
    %1383 = arith.ori %1381, %1382 : i1 loc(#loc94)
    %1384 = arith.andi %1370, %1383 : i1 loc(#loc94)
    acir.path %1371 {branch = 26 : i64, kind = "branch", txn = 5 : i64} loc(#loc94)
    %1385 = arith.xori %1374, %true : i1
    %1386 = arith.ori %1385, %1375 : i1
    %1387 = arith.ori %1374, %1371 : i1
    %1388 = arith.xori %1371, %true : i1
    %1389 = arith.ori %1388, %1377 : i1
    %1390 = arith.andi %1386, %1389 : i1
    %1391 = arith.andi %969, %1366 : i1
    %1392 = arith.xori %971, %true : i1
    %1393 = arith.ori %1392, %973 : i1
    %1394 = arith.ori %971, %969 : i1
    %1395 = arith.xori %969, %true : i1
    %1396 = arith.ori %1395, %1297 : i1
    %1397 = arith.andi %1393, %1396 : i1
    %1398 = acir.extract %963 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc98)
    %1399 = acir.extract %963 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc99)
    %1400 = arith.andi %962, %962 : i1 loc(#loc100)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc100)
    %true_91 = arith.constant {acir.source_type = "bool"} true
    %false_92 = arith.constant {acir.source_type = "bool"} false
    %value_93, %ready_94 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc101)
    %1401 = arith.andi %1384, %ready_94 : i1 loc(#loc101)
    %1402 = acir.extract %value_93 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc102)
    %1403 = arith.andi %1401, %ready_94 : i1
    %1404 = arith.xori %1402, %true : i1
    %1405 = arith.xori %1404, %true : i1
    %1406 = arith.ori %1405, %ready_94 : i1
    %1407 = arith.xori %1402, %true : i1
    %1408 = arith.ori %1407, %ready_94 : i1
    %1409 = acir.extract %value_93 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc103)
    %1410 = acir.extract %1409 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc103)
    %1411 = acir.unary %1410 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc104)
    %1412 = arith.xori %1402, %true : i1
    %1413 = arith.andi %1408, %ready_94 : i1
    %1414 = arith.ori %1412, %1413 : i1
    %1415 = arith.andi %1403, %1414 : i1
    %1416 = arith.andi %1402, %1411 : i1
    %1417 = arith.andi %1408, %ready_94 : i1
    %1418 = arith.xori %1411, %true : i1
    %1419 = arith.andi %1402, %1418 : i1
    %1420 = arith.andi %1408, %ready_94 : i1
    %1421 = arith.xori %1419, %true : i1
    %1422 = arith.ori %1421, %1420 : i1
    %1423 = arith.xori %1416, %true : i1
    %1424 = arith.ori %1423, %1417 : i1
    %1425 = acir.extract %value_93 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc105)
    %1426 = acir.extract %1425 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc105)
    %1427 = arith.andi %ready_94, %962 : i1 loc(#loc105)
    %1428 = arith.cmpi eq, %1426, %1398 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc105)
    %1429 = arith.xori %1419, %true : i1
    %1430 = arith.ori %1429, %1422 : i1
    %1431 = arith.ori %1419, %1416 : i1
    %1432 = arith.xori %1416, %true : i1
    %1433 = arith.ori %1432, %1424 : i1
    %1434 = arith.andi %1430, %1433 : i1
    %1435 = arith.andi %1419, %1422 : i1
    %1436 = arith.andi %1424, %1427 : i1
    %1437 = arith.andi %1416, %1436 : i1
    %1438 = arith.ori %1435, %1437 : i1
    %1439 = arith.select %1419, %false_92, %1428 : i1
    %1440 = arith.xori %1402, %true : i1
    %1441 = arith.ori %1440, %1408 : i1
    %1442 = arith.xori %1402, %true : i1
    %1443 = arith.andi %1441, %1438 : i1
    %1444 = arith.ori %1442, %1443 : i1
    %1445 = arith.andi %1415, %1444 : i1
    %1446 = arith.andi %1402, %1439 : i1
    %1447 = arith.andi %1441, %1438 : i1
    %1448 = arith.xori %1439, %true : i1
    %1449 = arith.andi %1402, %1448 : i1
    %1450 = arith.andi %1441, %1438 : i1
    %1451 = arith.xori %1449, %true : i1
    %1452 = arith.ori %1451, %1450 : i1
    %1453 = arith.xori %1446, %true : i1
    %1454 = arith.ori %1453, %1447 : i1
    %1455 = arith.andi %962, %962 : i1 loc(#loc106)
    %1456 = acir.aggregate(%true_91, %1398, %1399) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc106)
    %1457 = arith.andi %ready_94, %1455 : i1 loc(#loc107)
    %1458 = acir.update %value_93, %1456 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc107)
    %1459 = arith.xori %1449, %true : i1
    %1460 = arith.ori %1459, %1452 : i1
    %1461 = arith.ori %1449, %1446 : i1
    %1462 = arith.xori %1446, %true : i1
    %1463 = arith.ori %1462, %1454 : i1
    %1464 = arith.andi %1460, %1463 : i1
    %1465 = arith.andi %1452, %ready_94 : i1
    %1466 = arith.andi %1449, %1465 : i1
    %1467 = arith.andi %1454, %1457 : i1
    %1468 = arith.andi %1446, %1467 : i1
    %1469 = arith.ori %1466, %1468 : i1
    %1470 = arith.select %1449, %value_93, %1458 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1471 = arith.xori %1402, %true : i1
    %1472 = arith.ori %1471, %1441 : i1
    %1473 = acir.extract %1470 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc108)
    %1474 = acir.extract %1473 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc108)
    %1475 = acir.unary %1474 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc109)
    %1476 = arith.xori %1402, %true : i1
    %1477 = arith.andi %1472, %1469 : i1
    %1478 = arith.ori %1476, %1477 : i1
    %1479 = arith.andi %1445, %1478 : i1
    %1480 = arith.andi %1402, %1475 : i1
    %1481 = arith.andi %1472, %1469 : i1
    %1482 = arith.xori %1475, %true : i1
    %1483 = arith.andi %1402, %1482 : i1
    %1484 = arith.andi %1472, %1469 : i1
    %1485 = arith.xori %1483, %true : i1
    %1486 = arith.ori %1485, %1484 : i1
    %1487 = arith.xori %1480, %true : i1
    %1488 = arith.ori %1487, %1481 : i1
    %1489 = acir.extract %1470 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc110)
    %1490 = acir.extract %1489 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc110)
    %1491 = arith.andi %1469, %962 : i1 loc(#loc110)
    %1492 = arith.cmpi eq, %1490, %1398 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc110)
    %1493 = arith.xori %1483, %true : i1
    %1494 = arith.ori %1493, %1486 : i1
    %1495 = arith.ori %1483, %1480 : i1
    %1496 = arith.xori %1480, %true : i1
    %1497 = arith.ori %1496, %1488 : i1
    %1498 = arith.andi %1494, %1497 : i1
    %1499 = arith.andi %1483, %1486 : i1
    %1500 = arith.andi %1488, %1491 : i1
    %1501 = arith.andi %1480, %1500 : i1
    %1502 = arith.ori %1499, %1501 : i1
    %1503 = arith.select %1483, %false_92, %1492 : i1
    %1504 = arith.xori %1402, %true : i1
    %1505 = arith.ori %1504, %1472 : i1
    %1506 = arith.andi %1502, %1438 : i1
    %1507 = arith.select %1503, %true_91, %1439 : i1
    %1508 = arith.xori %1402, %true : i1
    %1509 = arith.andi %1505, %1502 : i1
    %1510 = arith.ori %1508, %1509 : i1
    %1511 = arith.andi %1479, %1510 : i1
    %1512 = arith.andi %1402, %1503 : i1
    %1513 = arith.andi %1505, %1502 : i1
    %1514 = arith.xori %1503, %true : i1
    %1515 = arith.andi %1402, %1514 : i1
    %1516 = arith.andi %1505, %1502 : i1
    %1517 = arith.xori %1515, %true : i1
    %1518 = arith.ori %1517, %1516 : i1
    %1519 = arith.xori %1512, %true : i1
    %1520 = arith.ori %1519, %1513 : i1
    %1521 = arith.andi %962, %962 : i1 loc(#loc111)
    %1522 = acir.aggregate(%true_91, %1398, %1399) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc111)
    %1523 = arith.andi %1469, %1521 : i1 loc(#loc112)
    %1524 = acir.update %1470, %1522 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc112)
    %1525 = arith.xori %1515, %true : i1
    %1526 = arith.ori %1525, %1518 : i1
    %1527 = arith.ori %1515, %1512 : i1
    %1528 = arith.xori %1512, %true : i1
    %1529 = arith.ori %1528, %1520 : i1
    %1530 = arith.andi %1526, %1529 : i1
    %1531 = arith.andi %1518, %1469 : i1
    %1532 = arith.andi %1515, %1531 : i1
    %1533 = arith.andi %1520, %1523 : i1
    %1534 = arith.andi %1512, %1533 : i1
    %1535 = arith.ori %1532, %1534 : i1
    %1536 = arith.select %1515, %1470, %1524 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1537 = arith.xori %1402, %true : i1
    %1538 = arith.ori %1537, %1505 : i1
    %1539 = arith.andi %1506, %1535 : i1
    %1540 = arith.xori %1404, %true : i1
    %1541 = arith.ori %1540, %1406 : i1
    %1542 = arith.ori %1404, %1402 : i1
    %1543 = arith.xori %1402, %true : i1
    %1544 = arith.ori %1543, %1538 : i1
    %1545 = arith.andi %1541, %1544 : i1
    %1546 = arith.andi %1404, %1406 : i1
    %1547 = arith.andi %1538, %1506 : i1
    %1548 = arith.andi %1402, %1547 : i1
    %1549 = arith.ori %1546, %1548 : i1
    %1550 = arith.select %1404, %false_92, %1507 : i1
    %1551 = arith.andi %1406, %ready_94 : i1
    %1552 = arith.andi %1404, %1551 : i1
    %1553 = arith.andi %1538, %1535 : i1
    %1554 = arith.andi %1402, %1553 : i1
    %1555 = arith.ori %1552, %1554 : i1
    %1556 = arith.select %1404, %value_93, %1536 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1557 = arith.andi %1511, %1549 : i1
    %1558 = arith.xori %1550, %true : i1
    %1559 = arith.xori %1550, %true : i1
    %1560 = arith.ori %1559, %1549 : i1
    %1561 = arith.xori %1550, %true : i1 loc(#loc113)
    %1562 = arith.andi %1560, %1555 : i1 loc(#loc113)
    %1563 = arith.ori %1561, %1562 : i1 loc(#loc113)
    %1564 = arith.andi %1557, %1563 : i1 loc(#loc113)
    acir.path %1550 {branch = 27 : i64, kind = "branch", txn = 5 : i64} loc(#loc113)
    %1565 = arith.xori %1558, %true : i1
    %1566 = arith.ori %1565, %1549 : i1
    %1567 = arith.ori %1558, %1550 : i1
    %1568 = arith.xori %1550, %true : i1
    %1569 = arith.ori %1568, %1560 : i1
    %1570 = arith.andi %1566, %1569 : i1
    %1571 = acir.extract %963 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc114)
    %1572 = acir.extract %963 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc115)
    %1573 = arith.andi %962, %962 : i1 loc(#loc116)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc116)
    %true_95 = arith.constant {acir.source_type = "bool"} true
    %false_96 = arith.constant {acir.source_type = "bool"} false
    %value_97, %ready_98 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc101)
    %1574 = arith.andi %1564, %ready_98 : i1 loc(#loc101)
    %1575 = acir.extract %value_97 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc102)
    %1576 = arith.andi %1574, %ready_98 : i1
    %1577 = arith.xori %1575, %true : i1
    %1578 = arith.xori %1577, %true : i1
    %1579 = arith.ori %1578, %ready_98 : i1
    %1580 = arith.xori %1575, %true : i1
    %1581 = arith.ori %1580, %ready_98 : i1
    %1582 = acir.extract %value_97 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc103)
    %1583 = acir.extract %1582 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc103)
    %1584 = acir.unary %1583 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc104)
    %1585 = arith.xori %1575, %true : i1
    %1586 = arith.andi %1581, %ready_98 : i1
    %1587 = arith.ori %1585, %1586 : i1
    %1588 = arith.andi %1576, %1587 : i1
    %1589 = arith.andi %1575, %1584 : i1
    %1590 = arith.andi %1581, %ready_98 : i1
    %1591 = arith.xori %1584, %true : i1
    %1592 = arith.andi %1575, %1591 : i1
    %1593 = arith.andi %1581, %ready_98 : i1
    %1594 = arith.xori %1592, %true : i1
    %1595 = arith.ori %1594, %1593 : i1
    %1596 = arith.xori %1589, %true : i1
    %1597 = arith.ori %1596, %1590 : i1
    %1598 = acir.extract %value_97 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc105)
    %1599 = acir.extract %1598 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc105)
    %1600 = arith.andi %ready_98, %962 : i1 loc(#loc105)
    %1601 = arith.cmpi eq, %1599, %1571 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc105)
    %1602 = arith.xori %1592, %true : i1
    %1603 = arith.ori %1602, %1595 : i1
    %1604 = arith.ori %1592, %1589 : i1
    %1605 = arith.xori %1589, %true : i1
    %1606 = arith.ori %1605, %1597 : i1
    %1607 = arith.andi %1603, %1606 : i1
    %1608 = arith.andi %1592, %1595 : i1
    %1609 = arith.andi %1597, %1600 : i1
    %1610 = arith.andi %1589, %1609 : i1
    %1611 = arith.ori %1608, %1610 : i1
    %1612 = arith.select %1592, %false_96, %1601 : i1
    %1613 = arith.xori %1575, %true : i1
    %1614 = arith.ori %1613, %1581 : i1
    %1615 = arith.xori %1575, %true : i1
    %1616 = arith.andi %1614, %1611 : i1
    %1617 = arith.ori %1615, %1616 : i1
    %1618 = arith.andi %1588, %1617 : i1
    %1619 = arith.andi %1575, %1612 : i1
    %1620 = arith.andi %1614, %1611 : i1
    %1621 = arith.xori %1612, %true : i1
    %1622 = arith.andi %1575, %1621 : i1
    %1623 = arith.andi %1614, %1611 : i1
    %1624 = arith.xori %1622, %true : i1
    %1625 = arith.ori %1624, %1623 : i1
    %1626 = arith.xori %1619, %true : i1
    %1627 = arith.ori %1626, %1620 : i1
    %1628 = arith.andi %962, %962 : i1 loc(#loc106)
    %1629 = acir.aggregate(%true_95, %1571, %1572) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc106)
    %1630 = arith.andi %ready_98, %1628 : i1 loc(#loc107)
    %1631 = acir.update %value_97, %1629 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc107)
    %1632 = arith.xori %1622, %true : i1
    %1633 = arith.ori %1632, %1625 : i1
    %1634 = arith.ori %1622, %1619 : i1
    %1635 = arith.xori %1619, %true : i1
    %1636 = arith.ori %1635, %1627 : i1
    %1637 = arith.andi %1633, %1636 : i1
    %1638 = arith.andi %1625, %ready_98 : i1
    %1639 = arith.andi %1622, %1638 : i1
    %1640 = arith.andi %1627, %1630 : i1
    %1641 = arith.andi %1619, %1640 : i1
    %1642 = arith.ori %1639, %1641 : i1
    %1643 = arith.select %1622, %value_97, %1631 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1644 = arith.xori %1575, %true : i1
    %1645 = arith.ori %1644, %1614 : i1
    %1646 = acir.extract %1643 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc108)
    %1647 = acir.extract %1646 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc108)
    %1648 = acir.unary %1647 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc109)
    %1649 = arith.xori %1575, %true : i1
    %1650 = arith.andi %1645, %1642 : i1
    %1651 = arith.ori %1649, %1650 : i1
    %1652 = arith.andi %1618, %1651 : i1
    %1653 = arith.andi %1575, %1648 : i1
    %1654 = arith.andi %1645, %1642 : i1
    %1655 = arith.xori %1648, %true : i1
    %1656 = arith.andi %1575, %1655 : i1
    %1657 = arith.andi %1645, %1642 : i1
    %1658 = arith.xori %1656, %true : i1
    %1659 = arith.ori %1658, %1657 : i1
    %1660 = arith.xori %1653, %true : i1
    %1661 = arith.ori %1660, %1654 : i1
    %1662 = acir.extract %1643 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc110)
    %1663 = acir.extract %1662 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc110)
    %1664 = arith.andi %1642, %962 : i1 loc(#loc110)
    %1665 = arith.cmpi eq, %1663, %1571 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc110)
    %1666 = arith.xori %1656, %true : i1
    %1667 = arith.ori %1666, %1659 : i1
    %1668 = arith.ori %1656, %1653 : i1
    %1669 = arith.xori %1653, %true : i1
    %1670 = arith.ori %1669, %1661 : i1
    %1671 = arith.andi %1667, %1670 : i1
    %1672 = arith.andi %1656, %1659 : i1
    %1673 = arith.andi %1661, %1664 : i1
    %1674 = arith.andi %1653, %1673 : i1
    %1675 = arith.ori %1672, %1674 : i1
    %1676 = arith.select %1656, %false_96, %1665 : i1
    %1677 = arith.xori %1575, %true : i1
    %1678 = arith.ori %1677, %1645 : i1
    %1679 = arith.andi %1675, %1611 : i1
    %1680 = arith.select %1676, %true_95, %1612 : i1
    %1681 = arith.xori %1575, %true : i1
    %1682 = arith.andi %1678, %1675 : i1
    %1683 = arith.ori %1681, %1682 : i1
    %1684 = arith.andi %1652, %1683 : i1
    %1685 = arith.andi %1575, %1676 : i1
    %1686 = arith.andi %1678, %1675 : i1
    %1687 = arith.xori %1676, %true : i1
    %1688 = arith.andi %1575, %1687 : i1
    %1689 = arith.andi %1678, %1675 : i1
    %1690 = arith.xori %1688, %true : i1
    %1691 = arith.ori %1690, %1689 : i1
    %1692 = arith.xori %1685, %true : i1
    %1693 = arith.ori %1692, %1686 : i1
    %1694 = arith.andi %962, %962 : i1 loc(#loc111)
    %1695 = acir.aggregate(%true_95, %1571, %1572) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc111)
    %1696 = arith.andi %1642, %1694 : i1 loc(#loc112)
    %1697 = acir.update %1643, %1695 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc112)
    %1698 = arith.xori %1688, %true : i1
    %1699 = arith.ori %1698, %1691 : i1
    %1700 = arith.ori %1688, %1685 : i1
    %1701 = arith.xori %1685, %true : i1
    %1702 = arith.ori %1701, %1693 : i1
    %1703 = arith.andi %1699, %1702 : i1
    %1704 = arith.andi %1691, %1642 : i1
    %1705 = arith.andi %1688, %1704 : i1
    %1706 = arith.andi %1693, %1696 : i1
    %1707 = arith.andi %1685, %1706 : i1
    %1708 = arith.ori %1705, %1707 : i1
    %1709 = arith.select %1688, %1643, %1697 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1710 = arith.xori %1575, %true : i1
    %1711 = arith.ori %1710, %1678 : i1
    %1712 = arith.andi %1679, %1708 : i1
    %1713 = arith.xori %1577, %true : i1
    %1714 = arith.ori %1713, %1579 : i1
    %1715 = arith.ori %1577, %1575 : i1
    %1716 = arith.xori %1575, %true : i1
    %1717 = arith.ori %1716, %1711 : i1
    %1718 = arith.andi %1714, %1717 : i1
    %1719 = arith.andi %1577, %1579 : i1
    %1720 = arith.andi %1711, %1679 : i1
    %1721 = arith.andi %1575, %1720 : i1
    %1722 = arith.ori %1719, %1721 : i1
    %1723 = arith.select %1577, %false_96, %1680 : i1
    %1724 = arith.andi %1579, %ready_98 : i1
    %1725 = arith.andi %1577, %1724 : i1
    %1726 = arith.andi %1711, %1708 : i1
    %1727 = arith.andi %1575, %1726 : i1
    %1728 = arith.ori %1725, %1727 : i1
    %1729 = arith.select %1577, %value_97, %1709 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1730 = arith.andi %1684, %1722 : i1
    %1731 = arith.xori %1723, %true : i1
    %1732 = arith.xori %1723, %true : i1
    %1733 = arith.ori %1732, %1722 : i1
    %1734 = arith.xori %1723, %true : i1 loc(#loc113)
    %1735 = arith.andi %1733, %1728 : i1 loc(#loc113)
    %1736 = arith.ori %1734, %1735 : i1 loc(#loc113)
    %1737 = arith.andi %1730, %1736 : i1 loc(#loc113)
    acir.path %1723 {branch = 28 : i64, kind = "branch", txn = 5 : i64} loc(#loc113)
    %1738 = arith.xori %1731, %true : i1
    %1739 = arith.ori %1738, %1722 : i1
    %1740 = arith.ori %1731, %1723 : i1
    %1741 = arith.xori %1723, %true : i1
    %1742 = arith.ori %1741, %1733 : i1
    %1743 = arith.andi %1739, %1742 : i1
    %1744 = arith.xori %962, %true : i1 loc(#loc117)
    acir.path %962 {branch = 29 : i64, kind = "branch", txn = 5 : i64} loc(#loc117)
    %1745 = acir.fire "top.cdb.broadcast#5" %1737 {successors = [2, 4, 8], txn = 5 : i64} loc(#loc117)
    %1746 = arith.andi %1745, %913 : i1 loc(#loc117)
    acir.effect %1746() {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> () loc(#loc75)
    %1747 = arith.andi %1745, %911 : i1 loc(#loc117)
    acir.effect %1747() {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> () loc(#loc76)
    %1748 = arith.andi %1745, %1071 : i1 loc(#loc117)
    acir.effect %1748(%1080) {branch = 20 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc94)
    %1749 = arith.andi %1745, %1167 : i1 loc(#loc117)
    acir.effect %1749(%1176) {branch = 22 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc94)
    %1750 = arith.andi %1745, %1269 : i1 loc(#loc117)
    acir.effect %1750(%1278) {branch = 24 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc94)
    %1751 = arith.andi %1745, %1371 : i1 loc(#loc117)
    acir.effect %1751(%1380) {branch = 26 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc94)
    %1752 = arith.andi %1745, %1550 : i1 loc(#loc117)
    acir.effect %1752(%1556) {branch = 27 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc113)
    %1753 = arith.andi %1745, %1723 : i1 loc(#loc117)
    acir.effect %1753(%1729) {branch = 28 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc113)
    %1754 = arith.andi %1745, %962 : i1 loc(#loc117)
    acir.effect %1754(%963) {branch = 29 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.event", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> () loc(#loc117)
    %false_99 = arith.constant {acir.source_type = "bool"} false
    %value_100, %ready_101 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.add_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc118)
    %1755 = arith.xori %value_100, %true : i1
    %1756 = arith.xori %1755, %true : i1
    %1757 = arith.ori %1756, %ready_101 : i1
    %1758 = arith.xori %value_100, %true : i1
    %1759 = arith.ori %1758, %ready_101 : i1
    acir.path %value_100 {branch = 30 : i64, kind = "branch", txn = -1 : i64} loc(#loc119)
    %value_102, %ready_103 = acir.read(%value_100) {branch = 30 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.mul_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc119)
    %1760 = arith.xori %1755, %true : i1
    %1761 = arith.ori %1760, %1757 : i1
    %1762 = arith.ori %1755, %value_100 : i1
    %1763 = arith.xori %value_100, %true : i1
    %1764 = arith.ori %1763, %1759 : i1
    %1765 = arith.andi %1761, %1764 : i1
    %1766 = arith.andi %1755, %1757 : i1
    %1767 = arith.andi %1759, %ready_103 : i1
    %1768 = arith.andi %value_100, %1767 : i1
    %1769 = arith.ori %1766, %1768 : i1
    %1770 = arith.select %1755, %false_99, %value_102 : i1
    %1771 = arith.xori %1770, %true : i1
    %1772 = arith.xori %1770, %true : i1
    %1773 = arith.ori %1772, %1769 : i1
    %1774 = arith.andi %1770, %1773 : i1 loc(#loc120)
    acir.path %1770 {branch = 31 : i64, kind = "branch", txn = 6 : i64} loc(#loc120)
    acir.path %1770 {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc120)
    %1775 = arith.xori %1770, %true : i1 loc(#loc120)
    %1776 = arith.ori %1775, %1773 : i1 loc(#loc120)
    %1777 = arith.andi %1770, %1773 : i1 loc(#loc120)
    %true_104 = arith.constant {acir.source_type = "bool"} true loc(#loc121)
    %value_105, %ready_106 = acir.read(%1770) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "peek", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, i1) loc(#loc122)
    %1778 = arith.xori %1770, %true : i1 loc(#loc122)
    %1779 = arith.andi %1776, %ready_106 : i1 loc(#loc122)
    %1780 = arith.ori %1778, %1779 : i1 loc(#loc122)
    %1781 = arith.andi %1774, %1780 : i1 loc(#loc122)
    %1782 = arith.xori %1770, %true : i1 loc(#loc122)
    %1783 = arith.andi %1776, %ready_106 : i1 loc(#loc122)
    %1784 = arith.ori %1782, %1783 : i1 loc(#loc122)
    %1785 = arith.andi %1781, %1784 : i1 loc(#loc122)
    %value_107, %ready_108 = acir.read(%1770) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "empty", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (i1, i1) loc(#loc122)
    %1786 = arith.xori %1770, %true : i1 loc(#loc122)
    %1787 = arith.andi %1776, %ready_108 : i1 loc(#loc122)
    %1788 = arith.ori %1786, %1787 : i1 loc(#loc122)
    %1789 = arith.andi %1785, %1788 : i1 loc(#loc122)
    %1790 = arith.xori %value_107, %true : i1 loc(#loc122)
    %1791 = arith.xori %1770, %true : i1 loc(#loc122)
    %1792 = arith.andi %1776, %1790 : i1 loc(#loc122)
    %1793 = arith.ori %1791, %1792 : i1 loc(#loc122)
    %1794 = arith.andi %1789, %1793 : i1 loc(#loc122)
    %1795 = acir.extract %value_105 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc123)
    acir.path %1770 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc124)
    %1796 = arith.xori %1770, %true : i1 loc(#loc124)
    %1797 = arith.ori %1796, %1776 : i1 loc(#loc124)
    %1798 = arith.andi %1776, %ready_106 : i1 loc(#loc124)
    %1799 = arith.andi %1770, %1798 : i1 loc(#loc124)
    %false_109 = arith.constant {acir.source_type = "bool"} false
    %true_110 = arith.constant {acir.source_type = "bool"} true
    %c0_i32_111 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc125)
    %1800 = arith.cmpi ult, %1795, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc126)
    %1801 = arith.xori %1770, %true : i1 loc(#loc127)
    %1802 = arith.andi %1797, %1799 : i1 loc(#loc127)
    %1803 = arith.ori %1801, %1802 : i1 loc(#loc127)
    %1804 = arith.andi %1794, %1803 : i1 loc(#loc127)
    %1805 = arith.cmpi eq, %1795, %c0_i32_111 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc128)
    %1806 = arith.andi %1799, %1799 : i1
    %1807 = arith.xori %1770, %true : i1
    %1808 = arith.andi %1797, %1799 : i1
    %1809 = arith.ori %1807, %1808 : i1
    %1810 = arith.andi %1804, %1809 : i1
    %1811 = arith.andi %1770, %1805 : i1
    %1812 = arith.andi %1797, %1799 : i1
    %1813 = arith.xori %1805, %true : i1
    %1814 = arith.andi %1770, %1813 : i1
    %1815 = arith.andi %1797, %1799 : i1
    %1816 = arith.xori %1814, %true : i1
    %1817 = arith.ori %1816, %1815 : i1
    %1818 = arith.andi %1815, %1799 : i1
    %1819 = arith.andi %1814, %1818 : i1
    acir.path %1814 {branch = 32 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    %value_112, %ready_113 = acir.read(%1814) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc129)
    %1820 = arith.xori %1814, %true : i1 loc(#loc129)
    %1821 = arith.andi %1817, %ready_113 : i1 loc(#loc129)
    %1822 = arith.ori %1820, %1821 : i1 loc(#loc129)
    %1823 = arith.andi %1810, %1822 : i1 loc(#loc129)
    %1824 = arith.andi %ready_113, %1819 : i1 loc(#loc129)
    %1825 = acir.extract %value_112[%1795] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc129)
    %value_114, %ready_115 = acir.read(%1814) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc130)
    %1826 = arith.xori %1814, %true : i1 loc(#loc130)
    %1827 = arith.andi %1817, %ready_115 : i1 loc(#loc130)
    %1828 = arith.ori %1826, %1827 : i1 loc(#loc130)
    %1829 = arith.andi %1823, %1828 : i1 loc(#loc130)
    %1830 = arith.andi %ready_115, %1819 : i1 loc(#loc130)
    %1831 = acir.extract %value_114[%1795] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc130)
    %1832 = arith.andi %1824, %1830 : i1 loc(#loc131)
    %1833 = arith.cmpi eq, %1825, %1831 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc131)
    %1834 = arith.andi %1832, %1824 : i1
    %1835 = arith.xori %1814, %true : i1
    %1836 = arith.andi %1817, %1832 : i1
    %1837 = arith.ori %1835, %1836 : i1
    %1838 = arith.andi %1829, %1837 : i1
    %1839 = arith.andi %1814, %1833 : i1
    %1840 = arith.andi %1817, %1832 : i1
    %1841 = arith.xori %1833, %true : i1
    %1842 = arith.andi %1814, %1841 : i1
    %1843 = arith.andi %1817, %1832 : i1
    %1844 = arith.xori %1842, %true : i1
    %1845 = arith.ori %1844, %1843 : i1
    %1846 = arith.andi %1843, %1824 : i1
    %1847 = arith.andi %1842, %1846 : i1
    acir.path %1842 {branch = 33 : i64, kind = "branch", txn = 6 : i64} loc(#loc132)
    acir.path %1842 {branch = 33 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc132)
    %1848 = arith.xori %1842, %true : i1 loc(#loc132)
    %1849 = arith.ori %1848, %1845 : i1 loc(#loc132)
    %1850 = arith.andi %1845, %1847 : i1 loc(#loc132)
    %1851 = arith.andi %1842, %1850 : i1 loc(#loc132)
    %c2_i32_116 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_117 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc133)
    %c3_i32_118 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc134)
    %c1_i32_119 = arith.constant {acir.source_type = "u32"} 1 : i32
    %1852 = arith.subi %1825, %c1_i32_119 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc135)
    %1853 = arith.andi %1852, %c3_i32_118 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc135)
    %1854 = arith.cmpi eq, %1853, %c0_i32_117 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc136)
    %1855 = arith.xori %1842, %true : i1
    %1856 = arith.andi %1849, %1851 : i1
    %1857 = arith.ori %1855, %1856 : i1
    %1858 = arith.andi %1838, %1857 : i1
    %1859 = arith.andi %1842, %1854 : i1
    %1860 = arith.andi %1849, %1851 : i1
    %1861 = arith.xori %1854, %true : i1
    %1862 = arith.andi %1842, %1861 : i1
    %1863 = arith.andi %1849, %1851 : i1
    %1864 = arith.xori %1862, %true : i1
    %1865 = arith.ori %1864, %1863 : i1
    %1866 = arith.cmpi eq, %1853, %c1_i32_119 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc137)
    %1867 = arith.xori %1862, %true : i1
    %1868 = arith.andi %1865, %1851 : i1
    %1869 = arith.ori %1867, %1868 : i1
    %1870 = arith.andi %1858, %1869 : i1
    %1871 = arith.andi %1862, %1866 : i1
    %1872 = arith.andi %1865, %1851 : i1
    %1873 = arith.xori %1866, %true : i1
    %1874 = arith.andi %1862, %1873 : i1
    %1875 = arith.andi %1865, %1851 : i1
    %1876 = arith.xori %1874, %true : i1
    %1877 = arith.ori %1876, %1875 : i1
    %1878 = arith.cmpi eq, %1853, %c2_i32_116 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc138)
    %1879 = arith.xori %1874, %true : i1
    %1880 = arith.andi %1877, %1851 : i1
    %1881 = arith.ori %1879, %1880 : i1
    %1882 = arith.andi %1870, %1881 : i1
    %1883 = arith.andi %1874, %1878 : i1
    %1884 = arith.andi %1877, %1851 : i1
    %1885 = arith.xori %1878, %true : i1
    %1886 = arith.andi %1874, %1885 : i1
    %1887 = arith.andi %1877, %1851 : i1
    %1888 = arith.xori %1886, %true : i1
    %1889 = arith.ori %1888, %1887 : i1
    acir.path %1886 {branch = 34 : i64, kind = "branch", txn = 6 : i64} loc(#loc139)
    acir.path %1886 {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc139)
    %1890 = arith.xori %1886, %true : i1 loc(#loc139)
    %1891 = arith.ori %1890, %1889 : i1 loc(#loc139)
    %value_120, %ready_121 = acir.read(%1886) {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %1892 = arith.xori %1886, %true : i1 loc(#loc140)
    %1893 = arith.andi %1891, %ready_121 : i1 loc(#loc140)
    %1894 = arith.ori %1892, %1893 : i1 loc(#loc140)
    %1895 = arith.andi %1882, %1894 : i1 loc(#loc140)
    %1896 = arith.andi %1891, %ready_121 : i1 loc(#loc141)
    %1897 = arith.andi %1886, %1896 : i1 loc(#loc141)
    %1898 = arith.xori %1883, %true : i1 loc(#loc142)
    %1899 = arith.ori %1898, %1884 : i1 loc(#loc142)
    acir.path %1883 {branch = 35 : i64, kind = "branch", txn = 6 : i64} loc(#loc143)
    acir.path %1883 {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc143)
    %1900 = arith.xori %1883, %true : i1 loc(#loc143)
    %1901 = arith.ori %1900, %1899 : i1 loc(#loc143)
    %value_122, %ready_123 = acir.read(%1883) {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %1902 = arith.xori %1883, %true : i1 loc(#loc140)
    %1903 = arith.andi %1901, %ready_123 : i1 loc(#loc140)
    %1904 = arith.ori %1902, %1903 : i1 loc(#loc140)
    %1905 = arith.andi %1895, %1904 : i1 loc(#loc140)
    %1906 = arith.andi %1901, %ready_123 : i1 loc(#loc141)
    %1907 = arith.andi %1883, %1906 : i1 loc(#loc141)
    %1908 = arith.xori %1871, %true : i1 loc(#loc144)
    %1909 = arith.ori %1908, %1872 : i1 loc(#loc144)
    acir.path %1871 {branch = 36 : i64, kind = "branch", txn = 6 : i64} loc(#loc145)
    acir.path %1871 {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc145)
    %1910 = arith.xori %1871, %true : i1 loc(#loc145)
    %1911 = arith.ori %1910, %1909 : i1 loc(#loc145)
    %value_124, %ready_125 = acir.read(%1871) {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %1912 = arith.xori %1871, %true : i1 loc(#loc140)
    %1913 = arith.andi %1911, %ready_125 : i1 loc(#loc140)
    %1914 = arith.ori %1912, %1913 : i1 loc(#loc140)
    %1915 = arith.andi %1905, %1914 : i1 loc(#loc140)
    %1916 = arith.andi %1911, %ready_125 : i1 loc(#loc141)
    %1917 = arith.andi %1871, %1916 : i1 loc(#loc141)
    %1918 = arith.xori %1859, %true : i1 loc(#loc146)
    %1919 = arith.ori %1918, %1860 : i1 loc(#loc146)
    acir.path %1859 {branch = 37 : i64, kind = "branch", txn = 6 : i64} loc(#loc147)
    acir.path %1859 {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc147)
    %1920 = arith.xori %1859, %true : i1 loc(#loc147)
    %1921 = arith.ori %1920, %1919 : i1 loc(#loc147)
    %value_126, %ready_127 = acir.read(%1859) {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %1922 = arith.xori %1859, %true : i1 loc(#loc140)
    %1923 = arith.andi %1921, %ready_127 : i1 loc(#loc140)
    %1924 = arith.ori %1922, %1923 : i1 loc(#loc140)
    %1925 = arith.andi %1915, %1924 : i1 loc(#loc140)
    %1926 = arith.andi %1921, %ready_127 : i1 loc(#loc141)
    %1927 = arith.andi %1859, %1926 : i1 loc(#loc141)
    %1928 = arith.andi %1889, %1897 : i1 loc(#loc148)
    %1929 = arith.andi %1886, %1928 : i1 loc(#loc148)
    %1930 = arith.andi %1899, %1907 : i1 loc(#loc148)
    %1931 = arith.andi %1883, %1930 : i1 loc(#loc148)
    %1932 = arith.ori %1929, %1931 : i1 loc(#loc148)
    %1933 = arith.andi %1909, %1917 : i1 loc(#loc148)
    %1934 = arith.andi %1871, %1933 : i1 loc(#loc148)
    %1935 = arith.ori %1932, %1934 : i1 loc(#loc148)
    %1936 = arith.andi %1919, %1927 : i1 loc(#loc148)
    %1937 = arith.andi %1859, %1936 : i1 loc(#loc148)
    %1938 = arith.ori %1935, %1937 : i1 loc(#loc148)
    %1939 = arith.select %1871, %value_124, %value_126 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
    %1940 = arith.select %1883, %value_122, %1939 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
    %1941 = arith.select %1886, %value_120, %1940 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
    %1942 = acir.extract %1941 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc149)
    %1943 = arith.xori %1842, %true : i1
    %1944 = arith.andi %1845, %1938 : i1
    %1945 = arith.ori %1943, %1944 : i1
    %1946 = arith.andi %1925, %1945 : i1
    %1947 = arith.andi %1842, %1942 : i1
    %1948 = arith.andi %1845, %1938 : i1
    %1949 = arith.xori %1942, %true : i1
    %1950 = arith.andi %1842, %1949 : i1
    %1951 = arith.andi %1845, %1938 : i1
    %1952 = arith.xori %1947, %true : i1
    %1953 = arith.ori %1952, %1948 : i1
    %1954 = acir.extract %1941 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc150)
    %1955 = arith.xori %1950, %true : i1
    %1956 = arith.ori %1955, %1951 : i1
    %1957 = arith.ori %1950, %1947 : i1
    %1958 = arith.xori %1947, %true : i1
    %1959 = arith.ori %1958, %1953 : i1
    %1960 = arith.andi %1956, %1959 : i1
    %1961 = arith.andi %1950, %1951 : i1
    %1962 = arith.andi %1953, %1938 : i1
    %1963 = arith.andi %1947, %1962 : i1
    %1964 = arith.ori %1961, %1963 : i1
    %1965 = arith.select %1950, %false_109, %1954 : i1
    %1966 = arith.xori %1842, %true : i1
    %1967 = arith.andi %1845, %1964 : i1
    %1968 = arith.ori %1966, %1967 : i1
    %1969 = arith.andi %1946, %1968 : i1
    %1970 = arith.andi %1842, %1965 : i1
    %1971 = arith.andi %1845, %1964 : i1
    %1972 = arith.xori %1965, %true : i1
    %1973 = arith.andi %1842, %1972 : i1
    %1974 = arith.andi %1845, %1964 : i1
    %1975 = arith.xori %1970, %true : i1
    %1976 = arith.ori %1975, %1971 : i1
    %1977 = acir.extract %1941 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc151)
    %1978 = arith.andi %1938, %1847 : i1 loc(#loc151)
    %1979 = arith.cmpi eq, %1977, %1825 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc151)
    %1980 = arith.xori %1973, %true : i1
    %1981 = arith.ori %1980, %1974 : i1
    %1982 = arith.ori %1973, %1970 : i1
    %1983 = arith.xori %1970, %true : i1
    %1984 = arith.ori %1983, %1976 : i1
    %1985 = arith.andi %1981, %1984 : i1
    %1986 = arith.andi %1973, %1974 : i1
    %1987 = arith.andi %1976, %1978 : i1
    %1988 = arith.andi %1970, %1987 : i1
    %1989 = arith.ori %1986, %1988 : i1
    %1990 = arith.select %1973, %false_109, %1979 : i1
    %1991 = arith.andi %1989, %1847 : i1
    %1992 = arith.xori %1842, %true : i1
    %1993 = arith.andi %1845, %1989 : i1
    %1994 = arith.ori %1992, %1993 : i1
    %1995 = arith.andi %1969, %1994 : i1
    %1996 = arith.andi %1842, %1990 : i1
    %1997 = arith.andi %1845, %1989 : i1
    %1998 = arith.xori %1990, %true : i1
    %1999 = arith.andi %1842, %1998 : i1
    %2000 = arith.andi %1845, %1989 : i1
    %2001 = arith.xori %1999, %true : i1
    %2002 = arith.ori %2001, %2000 : i1
    %2003 = arith.andi %2000, %1847 : i1
    %2004 = arith.andi %1999, %2003 : i1
    %2005 = acir.aggregate(%false_109, %1825, %c0_i32_111) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc152)
    %2006 = arith.xori %1996, %true : i1 loc(#loc153)
    %2007 = arith.ori %2006, %1997 : i1 loc(#loc153)
    %2008 = acir.extract %1941 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc154)
    %2009 = arith.andi %1847, %1938 : i1 loc(#loc155)
    %2010 = acir.aggregate(%true_110, %1825, %2008) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc155)
    %2011 = arith.xori %1839, %true : i1 loc(#loc156)
    %2012 = arith.ori %2011, %1840 : i1 loc(#loc156)
    acir.path %1839 {branch = 38 : i64, kind = "branch", txn = 6 : i64} loc(#loc157)
    %value_128, %ready_129 = acir.read(%1839) {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc157)
    %2013 = arith.xori %1839, %true : i1 loc(#loc157)
    %2014 = arith.andi %2012, %ready_129 : i1 loc(#loc157)
    %2015 = arith.ori %2013, %2014 : i1 loc(#loc157)
    %2016 = arith.andi %1995, %2015 : i1 loc(#loc157)
    %2017 = arith.andi %ready_129, %1819 : i1 loc(#loc157)
    %2018 = acir.extract %value_128[%1795] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc157)
    %2019 = arith.andi %1824, %2017 : i1 loc(#loc158)
    %2020 = acir.aggregate(%true_110, %1825, %2018) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
    %2021 = arith.xori %1811, %true : i1 loc(#loc159)
    %2022 = arith.ori %2021, %1812 : i1 loc(#loc159)
    %2023 = acir.aggregate(%true_110, %c0_i32_111, %c0_i32_111) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc160)
    %2024 = arith.andi %2002, %2004 : i1 loc(#loc161)
    %2025 = arith.andi %1999, %2024 : i1 loc(#loc161)
    %2026 = arith.andi %2007, %2009 : i1 loc(#loc161)
    %2027 = arith.andi %1996, %2026 : i1 loc(#loc161)
    %2028 = arith.ori %2025, %2027 : i1 loc(#loc161)
    %2029 = arith.andi %2012, %2019 : i1 loc(#loc161)
    %2030 = arith.andi %1839, %2029 : i1 loc(#loc161)
    %2031 = arith.ori %2028, %2030 : i1 loc(#loc161)
    %2032 = arith.andi %1811, %2022 : i1 loc(#loc161)
    %2033 = arith.ori %2031, %2032 : i1 loc(#loc161)
    %2034 = arith.select %1839, %2020, %2023 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc161)
    %2035 = arith.select %1996, %2010, %2034 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc161)
    %2036 = arith.select %1999, %2005, %2035 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc161)
    %2037 = acir.extract %value_105 {acir.id = "v7", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc162)
    acir.path %1770 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc163)
    %2038 = arith.xori %1770, %true : i1 loc(#loc163)
    %2039 = arith.ori %2038, %1776 : i1 loc(#loc163)
    %2040 = arith.andi %1776, %ready_106 : i1 loc(#loc163)
    %2041 = arith.andi %1770, %2040 : i1 loc(#loc163)
    %false_130 = arith.constant {acir.source_type = "bool"} false
    %true_131 = arith.constant {acir.source_type = "bool"} true
    %c0_i32_132 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32_133 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc125)
    %2042 = arith.cmpi ult, %2037, %c8_i32_133 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc126)
    %2043 = arith.xori %1770, %true : i1 loc(#loc127)
    %2044 = arith.andi %2039, %2041 : i1 loc(#loc127)
    %2045 = arith.ori %2043, %2044 : i1 loc(#loc127)
    %2046 = arith.andi %2016, %2045 : i1 loc(#loc127)
    %2047 = arith.cmpi eq, %2037, %c0_i32_132 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc128)
    %2048 = arith.andi %2041, %2041 : i1
    %2049 = arith.xori %1770, %true : i1
    %2050 = arith.andi %2039, %2041 : i1
    %2051 = arith.ori %2049, %2050 : i1
    %2052 = arith.andi %2046, %2051 : i1
    %2053 = arith.andi %1770, %2047 : i1
    %2054 = arith.andi %2039, %2041 : i1
    %2055 = arith.xori %2047, %true : i1
    %2056 = arith.andi %1770, %2055 : i1
    %2057 = arith.andi %2039, %2041 : i1
    %2058 = arith.xori %2056, %true : i1
    %2059 = arith.ori %2058, %2057 : i1
    %2060 = arith.andi %2057, %2041 : i1
    %2061 = arith.andi %2056, %2060 : i1
    acir.path %2056 {branch = 39 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    %value_134, %ready_135 = acir.read(%2056) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc129)
    %2062 = arith.xori %2056, %true : i1 loc(#loc129)
    %2063 = arith.andi %2059, %ready_135 : i1 loc(#loc129)
    %2064 = arith.ori %2062, %2063 : i1 loc(#loc129)
    %2065 = arith.andi %2052, %2064 : i1 loc(#loc129)
    %2066 = arith.andi %ready_135, %2061 : i1 loc(#loc129)
    %2067 = acir.extract %value_134[%2037] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc129)
    %value_136, %ready_137 = acir.read(%2056) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc130)
    %2068 = arith.xori %2056, %true : i1 loc(#loc130)
    %2069 = arith.andi %2059, %ready_137 : i1 loc(#loc130)
    %2070 = arith.ori %2068, %2069 : i1 loc(#loc130)
    %2071 = arith.andi %2065, %2070 : i1 loc(#loc130)
    %2072 = arith.andi %ready_137, %2061 : i1 loc(#loc130)
    %2073 = acir.extract %value_136[%2037] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc130)
    %2074 = arith.andi %2066, %2072 : i1 loc(#loc131)
    %2075 = arith.cmpi eq, %2067, %2073 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc131)
    %2076 = arith.andi %2074, %2066 : i1
    %2077 = arith.xori %2056, %true : i1
    %2078 = arith.andi %2059, %2074 : i1
    %2079 = arith.ori %2077, %2078 : i1
    %2080 = arith.andi %2071, %2079 : i1
    %2081 = arith.andi %2056, %2075 : i1
    %2082 = arith.andi %2059, %2074 : i1
    %2083 = arith.xori %2075, %true : i1
    %2084 = arith.andi %2056, %2083 : i1
    %2085 = arith.andi %2059, %2074 : i1
    %2086 = arith.xori %2084, %true : i1
    %2087 = arith.ori %2086, %2085 : i1
    %2088 = arith.andi %2085, %2066 : i1
    %2089 = arith.andi %2084, %2088 : i1
    acir.path %2084 {branch = 40 : i64, kind = "branch", txn = 6 : i64} loc(#loc132)
    acir.path %2084 {branch = 40 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc132)
    %2090 = arith.xori %2084, %true : i1 loc(#loc132)
    %2091 = arith.ori %2090, %2087 : i1 loc(#loc132)
    %2092 = arith.andi %2087, %2089 : i1 loc(#loc132)
    %2093 = arith.andi %2084, %2092 : i1 loc(#loc132)
    %c2_i32_138 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_139 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc133)
    %c3_i32_140 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc134)
    %c1_i32_141 = arith.constant {acir.source_type = "u32"} 1 : i32
    %2094 = arith.subi %2067, %c1_i32_141 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc135)
    %2095 = arith.andi %2094, %c3_i32_140 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc135)
    %2096 = arith.cmpi eq, %2095, %c0_i32_139 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc136)
    %2097 = arith.xori %2084, %true : i1
    %2098 = arith.andi %2091, %2093 : i1
    %2099 = arith.ori %2097, %2098 : i1
    %2100 = arith.andi %2080, %2099 : i1
    %2101 = arith.andi %2084, %2096 : i1
    %2102 = arith.andi %2091, %2093 : i1
    %2103 = arith.xori %2096, %true : i1
    %2104 = arith.andi %2084, %2103 : i1
    %2105 = arith.andi %2091, %2093 : i1
    %2106 = arith.xori %2104, %true : i1
    %2107 = arith.ori %2106, %2105 : i1
    %2108 = arith.cmpi eq, %2095, %c1_i32_141 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc137)
    %2109 = arith.xori %2104, %true : i1
    %2110 = arith.andi %2107, %2093 : i1
    %2111 = arith.ori %2109, %2110 : i1
    %2112 = arith.andi %2100, %2111 : i1
    %2113 = arith.andi %2104, %2108 : i1
    %2114 = arith.andi %2107, %2093 : i1
    %2115 = arith.xori %2108, %true : i1
    %2116 = arith.andi %2104, %2115 : i1
    %2117 = arith.andi %2107, %2093 : i1
    %2118 = arith.xori %2116, %true : i1
    %2119 = arith.ori %2118, %2117 : i1
    %2120 = arith.cmpi eq, %2095, %c2_i32_138 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc138)
    %2121 = arith.xori %2116, %true : i1
    %2122 = arith.andi %2119, %2093 : i1
    %2123 = arith.ori %2121, %2122 : i1
    %2124 = arith.andi %2112, %2123 : i1
    %2125 = arith.andi %2116, %2120 : i1
    %2126 = arith.andi %2119, %2093 : i1
    %2127 = arith.xori %2120, %true : i1
    %2128 = arith.andi %2116, %2127 : i1
    %2129 = arith.andi %2119, %2093 : i1
    %2130 = arith.xori %2128, %true : i1
    %2131 = arith.ori %2130, %2129 : i1
    acir.path %2128 {branch = 41 : i64, kind = "branch", txn = 6 : i64} loc(#loc139)
    acir.path %2128 {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc139)
    %2132 = arith.xori %2128, %true : i1 loc(#loc139)
    %2133 = arith.ori %2132, %2131 : i1 loc(#loc139)
    %value_142, %ready_143 = acir.read(%2128) {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %2134 = arith.xori %2128, %true : i1 loc(#loc140)
    %2135 = arith.andi %2133, %ready_143 : i1 loc(#loc140)
    %2136 = arith.ori %2134, %2135 : i1 loc(#loc140)
    %2137 = arith.andi %2124, %2136 : i1 loc(#loc140)
    %2138 = arith.andi %2133, %ready_143 : i1 loc(#loc141)
    %2139 = arith.andi %2128, %2138 : i1 loc(#loc141)
    %2140 = arith.xori %2125, %true : i1 loc(#loc142)
    %2141 = arith.ori %2140, %2126 : i1 loc(#loc142)
    acir.path %2125 {branch = 42 : i64, kind = "branch", txn = 6 : i64} loc(#loc143)
    acir.path %2125 {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc143)
    %2142 = arith.xori %2125, %true : i1 loc(#loc143)
    %2143 = arith.ori %2142, %2141 : i1 loc(#loc143)
    %value_144, %ready_145 = acir.read(%2125) {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %2144 = arith.xori %2125, %true : i1 loc(#loc140)
    %2145 = arith.andi %2143, %ready_145 : i1 loc(#loc140)
    %2146 = arith.ori %2144, %2145 : i1 loc(#loc140)
    %2147 = arith.andi %2137, %2146 : i1 loc(#loc140)
    %2148 = arith.andi %2143, %ready_145 : i1 loc(#loc141)
    %2149 = arith.andi %2125, %2148 : i1 loc(#loc141)
    %2150 = arith.xori %2113, %true : i1 loc(#loc144)
    %2151 = arith.ori %2150, %2114 : i1 loc(#loc144)
    acir.path %2113 {branch = 43 : i64, kind = "branch", txn = 6 : i64} loc(#loc145)
    acir.path %2113 {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc145)
    %2152 = arith.xori %2113, %true : i1 loc(#loc145)
    %2153 = arith.ori %2152, %2151 : i1 loc(#loc145)
    %value_146, %ready_147 = acir.read(%2113) {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %2154 = arith.xori %2113, %true : i1 loc(#loc140)
    %2155 = arith.andi %2153, %ready_147 : i1 loc(#loc140)
    %2156 = arith.ori %2154, %2155 : i1 loc(#loc140)
    %2157 = arith.andi %2147, %2156 : i1 loc(#loc140)
    %2158 = arith.andi %2153, %ready_147 : i1 loc(#loc141)
    %2159 = arith.andi %2113, %2158 : i1 loc(#loc141)
    %2160 = arith.xori %2101, %true : i1 loc(#loc146)
    %2161 = arith.ori %2160, %2102 : i1 loc(#loc146)
    acir.path %2101 {branch = 44 : i64, kind = "branch", txn = 6 : i64} loc(#loc147)
    acir.path %2101 {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc147)
    %2162 = arith.xori %2101, %true : i1 loc(#loc147)
    %2163 = arith.ori %2162, %2161 : i1 loc(#loc147)
    %value_148, %ready_149 = acir.read(%2101) {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc140)
    %2164 = arith.xori %2101, %true : i1 loc(#loc140)
    %2165 = arith.andi %2163, %ready_149 : i1 loc(#loc140)
    %2166 = arith.ori %2164, %2165 : i1 loc(#loc140)
    %2167 = arith.andi %2157, %2166 : i1 loc(#loc140)
    %2168 = arith.andi %2163, %ready_149 : i1 loc(#loc141)
    %2169 = arith.andi %2101, %2168 : i1 loc(#loc141)
    %2170 = arith.andi %2131, %2139 : i1 loc(#loc148)
    %2171 = arith.andi %2128, %2170 : i1 loc(#loc148)
    %2172 = arith.andi %2141, %2149 : i1 loc(#loc148)
    %2173 = arith.andi %2125, %2172 : i1 loc(#loc148)
    %2174 = arith.ori %2171, %2173 : i1 loc(#loc148)
    %2175 = arith.andi %2151, %2159 : i1 loc(#loc148)
    %2176 = arith.andi %2113, %2175 : i1 loc(#loc148)
    %2177 = arith.ori %2174, %2176 : i1 loc(#loc148)
    %2178 = arith.andi %2161, %2169 : i1 loc(#loc148)
    %2179 = arith.andi %2101, %2178 : i1 loc(#loc148)
    %2180 = arith.ori %2177, %2179 : i1 loc(#loc148)
    %2181 = arith.select %2113, %value_146, %value_148 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
    %2182 = arith.select %2125, %value_144, %2181 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
    %2183 = arith.select %2128, %value_142, %2182 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
    %2184 = acir.extract %2183 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc149)
    %2185 = arith.xori %2084, %true : i1
    %2186 = arith.andi %2087, %2180 : i1
    %2187 = arith.ori %2185, %2186 : i1
    %2188 = arith.andi %2167, %2187 : i1
    %2189 = arith.andi %2084, %2184 : i1
    %2190 = arith.andi %2087, %2180 : i1
    %2191 = arith.xori %2184, %true : i1
    %2192 = arith.andi %2084, %2191 : i1
    %2193 = arith.andi %2087, %2180 : i1
    %2194 = arith.xori %2189, %true : i1
    %2195 = arith.ori %2194, %2190 : i1
    %2196 = acir.extract %2183 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc150)
    %2197 = arith.xori %2192, %true : i1
    %2198 = arith.ori %2197, %2193 : i1
    %2199 = arith.ori %2192, %2189 : i1
    %2200 = arith.xori %2189, %true : i1
    %2201 = arith.ori %2200, %2195 : i1
    %2202 = arith.andi %2198, %2201 : i1
    %2203 = arith.andi %2192, %2193 : i1
    %2204 = arith.andi %2195, %2180 : i1
    %2205 = arith.andi %2189, %2204 : i1
    %2206 = arith.ori %2203, %2205 : i1
    %2207 = arith.select %2192, %false_130, %2196 : i1
    %2208 = arith.xori %2084, %true : i1
    %2209 = arith.andi %2087, %2206 : i1
    %2210 = arith.ori %2208, %2209 : i1
    %2211 = arith.andi %2188, %2210 : i1
    %2212 = arith.andi %2084, %2207 : i1
    %2213 = arith.andi %2087, %2206 : i1
    %2214 = arith.xori %2207, %true : i1
    %2215 = arith.andi %2084, %2214 : i1
    %2216 = arith.andi %2087, %2206 : i1
    %2217 = arith.xori %2212, %true : i1
    %2218 = arith.ori %2217, %2213 : i1
    %2219 = acir.extract %2183 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc151)
    %2220 = arith.andi %2180, %2089 : i1 loc(#loc151)
    %2221 = arith.cmpi eq, %2219, %2067 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc151)
    %2222 = arith.xori %2215, %true : i1
    %2223 = arith.ori %2222, %2216 : i1
    %2224 = arith.ori %2215, %2212 : i1
    %2225 = arith.xori %2212, %true : i1
    %2226 = arith.ori %2225, %2218 : i1
    %2227 = arith.andi %2223, %2226 : i1
    %2228 = arith.andi %2215, %2216 : i1
    %2229 = arith.andi %2218, %2220 : i1
    %2230 = arith.andi %2212, %2229 : i1
    %2231 = arith.ori %2228, %2230 : i1
    %2232 = arith.select %2215, %false_130, %2221 : i1
    %2233 = arith.andi %2231, %2089 : i1
    %2234 = arith.xori %2084, %true : i1
    %2235 = arith.andi %2087, %2231 : i1
    %2236 = arith.ori %2234, %2235 : i1
    %2237 = arith.andi %2211, %2236 : i1
    %2238 = arith.andi %2084, %2232 : i1
    %2239 = arith.andi %2087, %2231 : i1
    %2240 = arith.xori %2232, %true : i1
    %2241 = arith.andi %2084, %2240 : i1
    %2242 = arith.andi %2087, %2231 : i1
    %2243 = arith.xori %2241, %true : i1
    %2244 = arith.ori %2243, %2242 : i1
    %2245 = arith.andi %2242, %2089 : i1
    %2246 = arith.andi %2241, %2245 : i1
    %2247 = acir.aggregate(%false_130, %2067, %c0_i32_132) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc152)
    %2248 = arith.xori %2238, %true : i1 loc(#loc153)
    %2249 = arith.ori %2248, %2239 : i1 loc(#loc153)
    %2250 = acir.extract %2183 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc154)
    %2251 = arith.andi %2089, %2180 : i1 loc(#loc155)
    %2252 = acir.aggregate(%true_131, %2067, %2250) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc155)
    %2253 = arith.xori %2081, %true : i1 loc(#loc156)
    %2254 = arith.ori %2253, %2082 : i1 loc(#loc156)
    acir.path %2081 {branch = 45 : i64, kind = "branch", txn = 6 : i64} loc(#loc157)
    %value_150, %ready_151 = acir.read(%2081) {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc157)
    %2255 = arith.xori %2081, %true : i1 loc(#loc157)
    %2256 = arith.andi %2254, %ready_151 : i1 loc(#loc157)
    %2257 = arith.ori %2255, %2256 : i1 loc(#loc157)
    %2258 = arith.andi %2237, %2257 : i1 loc(#loc157)
    %2259 = arith.andi %ready_151, %2061 : i1 loc(#loc157)
    %2260 = acir.extract %value_150[%2037] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc157)
    %2261 = arith.andi %2066, %2259 : i1 loc(#loc158)
    %2262 = acir.aggregate(%true_131, %2067, %2260) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
    %2263 = arith.xori %2053, %true : i1 loc(#loc159)
    %2264 = arith.ori %2263, %2054 : i1 loc(#loc159)
    %2265 = acir.aggregate(%true_131, %c0_i32_132, %c0_i32_132) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc160)
    %2266 = arith.andi %2244, %2246 : i1 loc(#loc161)
    %2267 = arith.andi %2241, %2266 : i1 loc(#loc161)
    %2268 = arith.andi %2249, %2251 : i1 loc(#loc161)
    %2269 = arith.andi %2238, %2268 : i1 loc(#loc161)
    %2270 = arith.ori %2267, %2269 : i1 loc(#loc161)
    %2271 = arith.andi %2254, %2261 : i1 loc(#loc161)
    %2272 = arith.andi %2081, %2271 : i1 loc(#loc161)
    %2273 = arith.ori %2270, %2272 : i1 loc(#loc161)
    %2274 = arith.andi %2053, %2264 : i1 loc(#loc161)
    %2275 = arith.ori %2273, %2274 : i1 loc(#loc161)
    %2276 = arith.select %2081, %2262, %2265 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc161)
    %2277 = arith.select %2238, %2252, %2276 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc161)
    %2278 = arith.select %2241, %2247, %2277 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc161)
    %2279 = acir.extract %value_105 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc164)
    acir.path %1770 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc165)
    %2280 = arith.xori %1770, %true : i1 loc(#loc165)
    %2281 = arith.ori %2280, %1776 : i1 loc(#loc165)
    %2282 = arith.andi %1776, %ready_106 : i1 loc(#loc165)
    %2283 = arith.andi %1770, %2282 : i1 loc(#loc165)
    %2284 = arith.xori %1770, %true : i1 loc(#loc165)
    %2285 = arith.ori %2284, %2281 : i1 loc(#loc165)
    %2286 = arith.andi %2281, %2283 : i1 loc(#loc165)
    %2287 = arith.andi %1770, %2286 : i1 loc(#loc165)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc166)
    %value_152, %ready_153 = acir.read(%1770) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc167)
    %2288 = arith.xori %1770, %true : i1 loc(#loc167)
    %2289 = arith.andi %2285, %ready_153 : i1 loc(#loc167)
    %2290 = arith.ori %2288, %2289 : i1 loc(#loc167)
    %2291 = arith.andi %2258, %2290 : i1 loc(#loc167)
    %value_154, %ready_155 = acir.read(%1770) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc168)
    %2292 = arith.xori %1770, %true : i1 loc(#loc168)
    %2293 = arith.andi %2285, %ready_155 : i1 loc(#loc168)
    %2294 = arith.ori %2292, %2293 : i1 loc(#loc168)
    %2295 = arith.andi %2291, %2294 : i1 loc(#loc168)
    %2296 = arith.andi %ready_153, %ready_155 : i1 loc(#loc167)
    %2297 = arith.subi %value_152, %value_154 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc167)
    %2298 = arith.cmpi ult, %2297, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc167)
    %2299 = arith.andi %2285, %2296 : i1 loc(#loc169)
    %2300 = arith.andi %1770, %2299 : i1 loc(#loc169)
    %2301 = arith.andi %2281, %2300 : i1 loc(#loc170)
    %2302 = arith.andi %1770, %2301 : i1 loc(#loc170)
    %2303 = arith.andi %2302, %2298 : i1 loc(#loc170)
    %2304 = arith.xori %1770, %true : i1 loc(#loc170)
    %2305 = arith.andi %1776, %2303 : i1 loc(#loc170)
    %2306 = arith.ori %2304, %2305 : i1 loc(#loc170)
    %2307 = arith.andi %2295, %2306 : i1 loc(#loc170)
    %2308 = arith.andi %1776, %2303 : i1 loc(#loc170)
    %2309 = arith.xori %1770, %true : i1 loc(#loc170)
    %2310 = arith.ori %2309, %2308 : i1 loc(#loc170)
    %2311 = arith.andi %2308, %ready_106 : i1 loc(#loc170)
    %2312 = arith.andi %1770, %2311 : i1 loc(#loc170)
    %c2_i32_156 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_157 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc171)
    %c3_i32_158 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc172)
    %c1_i32_159 = arith.constant {acir.source_type = "u32"} 1 : i32
    %value_160, %ready_161 = acir.read(%1770) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc173)
    %2313 = arith.xori %1770, %true : i1 loc(#loc173)
    %2314 = arith.andi %2310, %ready_161 : i1 loc(#loc173)
    %2315 = arith.ori %2313, %2314 : i1 loc(#loc173)
    %2316 = arith.andi %2307, %2315 : i1 loc(#loc173)
    %2317 = arith.subi %value_160, %c1_i32_159 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc174)
    %2318 = arith.andi %2317, %c3_i32_158 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc174)
    %2319 = arith.cmpi eq, %2318, %c0_i32_157 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc175)
    %2320 = arith.xori %1770, %true : i1
    %2321 = arith.andi %2310, %ready_161 : i1
    %2322 = arith.ori %2320, %2321 : i1
    %2323 = arith.andi %2316, %2322 : i1
    %2324 = arith.andi %1770, %2319 : i1
    %2325 = arith.andi %2310, %ready_161 : i1
    %2326 = arith.xori %2319, %true : i1
    %2327 = arith.andi %1770, %2326 : i1
    %2328 = arith.andi %2310, %ready_161 : i1
    %2329 = arith.xori %2327, %true : i1
    %2330 = arith.ori %2329, %2328 : i1
    %2331 = arith.cmpi eq, %2318, %c1_i32_159 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc176)
    %2332 = arith.xori %2327, %true : i1
    %2333 = arith.andi %2330, %ready_161 : i1
    %2334 = arith.ori %2332, %2333 : i1
    %2335 = arith.andi %2323, %2334 : i1
    %2336 = arith.andi %2327, %2331 : i1
    %2337 = arith.andi %2330, %ready_161 : i1
    %2338 = arith.xori %2331, %true : i1
    %2339 = arith.andi %2327, %2338 : i1
    %2340 = arith.andi %2330, %ready_161 : i1
    %2341 = arith.xori %2339, %true : i1
    %2342 = arith.ori %2341, %2340 : i1
    %2343 = arith.cmpi eq, %2318, %c2_i32_156 {acir.id = "v20", acir.source_type = "bool"} : i32 loc(#loc177)
    %2344 = arith.xori %2339, %true : i1
    %2345 = arith.andi %2342, %ready_161 : i1
    %2346 = arith.ori %2344, %2345 : i1
    %2347 = arith.andi %2335, %2346 : i1
    %2348 = arith.andi %2339, %2343 : i1
    %2349 = arith.andi %2342, %ready_161 : i1
    %2350 = arith.xori %2343, %true : i1
    %2351 = arith.andi %2339, %2350 : i1
    %2352 = arith.andi %2342, %ready_161 : i1
    %2353 = arith.xori %2351, %true : i1
    %2354 = arith.ori %2353, %2352 : i1
    %2355 = arith.andi %ready_161, %2312 : i1 loc(#loc178)
    acir.path %2351 {branch = 46 : i64, kind = "branch", txn = 6 : i64} loc(#loc178)
    acir.path %2351 {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc178)
    %2356 = arith.xori %2351, %true : i1 loc(#loc178)
    %2357 = arith.ori %2356, %2354 : i1 loc(#loc178)
    %2358 = arith.andi %2354, %ready_161 : i1 loc(#loc178)
    %2359 = arith.andi %2351, %2358 : i1 loc(#loc178)
    %2360 = arith.andi %2354, %2312 : i1 loc(#loc178)
    %2361 = arith.andi %2351, %2360 : i1 loc(#loc178)
    %2362 = arith.andi %2359, %2361 : i1 loc(#loc178)
    %2363 = arith.xori %2351, %true : i1 loc(#loc178)
    %2364 = arith.ori %2363, %2357 : i1 loc(#loc178)
    %2365 = arith.andi %2357, %2359 : i1 loc(#loc178)
    %2366 = arith.andi %2351, %2365 : i1 loc(#loc178)
    %2367 = arith.andi %2357, %2361 : i1 loc(#loc178)
    %2368 = arith.andi %2351, %2367 : i1 loc(#loc178)
    %value_162, %ready_163 = acir.read(%2351) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc179)
    %2369 = arith.xori %2351, %true : i1 loc(#loc179)
    %2370 = arith.andi %2364, %ready_163 : i1 loc(#loc179)
    %2371 = arith.ori %2369, %2370 : i1 loc(#loc179)
    %2372 = arith.andi %2347, %2371 : i1 loc(#loc179)
    %2373 = acir.extract %value_162 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc179)
    %2374 = acir.unary %2373 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc180)
    %2375 = arith.andi %2364, %ready_163 : i1 loc(#loc181)
    %2376 = arith.andi %2351, %2375 : i1 loc(#loc181)
    %2377 = arith.andi %2357, %2376 : i1 loc(#loc182)
    %2378 = arith.andi %2351, %2377 : i1 loc(#loc182)
    %2379 = arith.andi %2378, %2374 : i1 loc(#loc182)
    %2380 = arith.xori %2351, %true : i1 loc(#loc182)
    %2381 = arith.andi %2354, %2379 : i1 loc(#loc182)
    %2382 = arith.ori %2380, %2381 : i1 loc(#loc182)
    %2383 = arith.andi %2372, %2382 : i1 loc(#loc182)
    %2384 = arith.andi %2354, %2379 : i1 loc(#loc182)
    %2385 = arith.xori %2351, %true : i1 loc(#loc182)
    %2386 = arith.ori %2385, %2384 : i1 loc(#loc182)
    %2387 = arith.andi %2384, %ready_161 : i1 loc(#loc182)
    %2388 = arith.andi %2351, %2387 : i1 loc(#loc182)
    %2389 = arith.andi %2384, %2312 : i1 loc(#loc182)
    %2390 = arith.andi %2351, %2389 : i1 loc(#loc182)
    %true_164 = arith.constant {acir.source_type = "bool"} true loc(#loc183)
    %false_165 = arith.constant {acir.source_type = "bool"} false loc(#loc184)
    %c0_i32_166 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc185)
    %2391 = arith.andi %2388, %2390 : i1 loc(#loc186)
    %2392 = acir.aggregate(%true_164, %false_165, %value_160, %2279, %c0_i32_166) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc186)
    %2393 = arith.xori %2351, %true : i1 loc(#loc187)
    %2394 = arith.andi %2386, %2391 : i1 loc(#loc187)
    %2395 = arith.ori %2393, %2394 : i1 loc(#loc187)
    %2396 = arith.andi %2383, %2395 : i1 loc(#loc187)
    %2397 = arith.andi %2351, %2386 : i1
    %2398 = arith.andi %2397, %2379 : i1
    %2399 = arith.xori %2348, %true : i1
    %2400 = arith.ori %2399, %2349 : i1
    %2401 = arith.andi %ready_161, %2312 : i1 loc(#loc188)
    acir.path %2348 {branch = 47 : i64, kind = "branch", txn = 6 : i64} loc(#loc188)
    acir.path %2348 {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc188)
    %2402 = arith.xori %2348, %true : i1 loc(#loc188)
    %2403 = arith.ori %2402, %2400 : i1 loc(#loc188)
    %2404 = arith.andi %2400, %ready_161 : i1 loc(#loc188)
    %2405 = arith.andi %2348, %2404 : i1 loc(#loc188)
    %2406 = arith.andi %2400, %2312 : i1 loc(#loc188)
    %2407 = arith.andi %2348, %2406 : i1 loc(#loc188)
    %2408 = arith.andi %2405, %2407 : i1 loc(#loc188)
    %2409 = arith.xori %2348, %true : i1 loc(#loc188)
    %2410 = arith.ori %2409, %2403 : i1 loc(#loc188)
    %2411 = arith.andi %2403, %2405 : i1 loc(#loc188)
    %2412 = arith.andi %2348, %2411 : i1 loc(#loc188)
    %2413 = arith.andi %2403, %2407 : i1 loc(#loc188)
    %2414 = arith.andi %2348, %2413 : i1 loc(#loc188)
    %value_167, %ready_168 = acir.read(%2348) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc179)
    %2415 = arith.xori %2348, %true : i1 loc(#loc179)
    %2416 = arith.andi %2410, %ready_168 : i1 loc(#loc179)
    %2417 = arith.ori %2415, %2416 : i1 loc(#loc179)
    %2418 = arith.andi %2396, %2417 : i1 loc(#loc179)
    %2419 = acir.extract %value_167 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc179)
    %2420 = acir.unary %2419 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc180)
    %2421 = arith.andi %2410, %ready_168 : i1 loc(#loc181)
    %2422 = arith.andi %2348, %2421 : i1 loc(#loc181)
    %2423 = arith.andi %2403, %2422 : i1 loc(#loc182)
    %2424 = arith.andi %2348, %2423 : i1 loc(#loc182)
    %2425 = arith.andi %2424, %2420 : i1 loc(#loc182)
    %2426 = arith.xori %2348, %true : i1 loc(#loc182)
    %2427 = arith.andi %2400, %2425 : i1 loc(#loc182)
    %2428 = arith.ori %2426, %2427 : i1 loc(#loc182)
    %2429 = arith.andi %2418, %2428 : i1 loc(#loc182)
    %2430 = arith.andi %2400, %2425 : i1 loc(#loc182)
    %2431 = arith.xori %2348, %true : i1 loc(#loc182)
    %2432 = arith.ori %2431, %2430 : i1 loc(#loc182)
    %2433 = arith.andi %2430, %ready_161 : i1 loc(#loc182)
    %2434 = arith.andi %2348, %2433 : i1 loc(#loc182)
    %2435 = arith.andi %2430, %2312 : i1 loc(#loc182)
    %2436 = arith.andi %2348, %2435 : i1 loc(#loc182)
    %true_169 = arith.constant {acir.source_type = "bool"} true loc(#loc183)
    %false_170 = arith.constant {acir.source_type = "bool"} false loc(#loc184)
    %c0_i32_171 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc185)
    %2437 = arith.andi %2434, %2436 : i1 loc(#loc186)
    %2438 = acir.aggregate(%true_169, %false_170, %value_160, %2279, %c0_i32_171) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc186)
    %2439 = arith.xori %2348, %true : i1 loc(#loc187)
    %2440 = arith.andi %2432, %2437 : i1 loc(#loc187)
    %2441 = arith.ori %2439, %2440 : i1 loc(#loc187)
    %2442 = arith.andi %2429, %2441 : i1 loc(#loc187)
    %2443 = arith.andi %2348, %2432 : i1
    %2444 = arith.andi %2443, %2425 : i1
    %2445 = arith.xori %2351, %true : i1
    %2446 = arith.ori %2445, %2354 : i1
    %2447 = arith.ori %2351, %2348 : i1
    %2448 = arith.xori %2348, %true : i1
    %2449 = arith.ori %2448, %2400 : i1
    %2450 = arith.andi %2446, %2449 : i1
    %2451 = arith.xori %2336, %true : i1
    %2452 = arith.ori %2451, %2337 : i1
    %2453 = arith.andi %ready_161, %2312 : i1 loc(#loc189)
    acir.path %2336 {branch = 48 : i64, kind = "branch", txn = 6 : i64} loc(#loc189)
    acir.path %2336 {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc189)
    %2454 = arith.xori %2336, %true : i1 loc(#loc189)
    %2455 = arith.ori %2454, %2452 : i1 loc(#loc189)
    %2456 = arith.andi %2452, %ready_161 : i1 loc(#loc189)
    %2457 = arith.andi %2336, %2456 : i1 loc(#loc189)
    %2458 = arith.andi %2452, %2312 : i1 loc(#loc189)
    %2459 = arith.andi %2336, %2458 : i1 loc(#loc189)
    %2460 = arith.andi %2457, %2459 : i1 loc(#loc189)
    %2461 = arith.xori %2336, %true : i1 loc(#loc189)
    %2462 = arith.ori %2461, %2455 : i1 loc(#loc189)
    %2463 = arith.andi %2455, %2457 : i1 loc(#loc189)
    %2464 = arith.andi %2336, %2463 : i1 loc(#loc189)
    %2465 = arith.andi %2455, %2459 : i1 loc(#loc189)
    %2466 = arith.andi %2336, %2465 : i1 loc(#loc189)
    %value_172, %ready_173 = acir.read(%2336) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc179)
    %2467 = arith.xori %2336, %true : i1 loc(#loc179)
    %2468 = arith.andi %2462, %ready_173 : i1 loc(#loc179)
    %2469 = arith.ori %2467, %2468 : i1 loc(#loc179)
    %2470 = arith.andi %2442, %2469 : i1 loc(#loc179)
    %2471 = acir.extract %value_172 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc179)
    %2472 = acir.unary %2471 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc180)
    %2473 = arith.andi %2462, %ready_173 : i1 loc(#loc181)
    %2474 = arith.andi %2336, %2473 : i1 loc(#loc181)
    %2475 = arith.andi %2455, %2474 : i1 loc(#loc182)
    %2476 = arith.andi %2336, %2475 : i1 loc(#loc182)
    %2477 = arith.andi %2476, %2472 : i1 loc(#loc182)
    %2478 = arith.xori %2336, %true : i1 loc(#loc182)
    %2479 = arith.andi %2452, %2477 : i1 loc(#loc182)
    %2480 = arith.ori %2478, %2479 : i1 loc(#loc182)
    %2481 = arith.andi %2470, %2480 : i1 loc(#loc182)
    %2482 = arith.andi %2452, %2477 : i1 loc(#loc182)
    %2483 = arith.xori %2336, %true : i1 loc(#loc182)
    %2484 = arith.ori %2483, %2482 : i1 loc(#loc182)
    %2485 = arith.andi %2482, %ready_161 : i1 loc(#loc182)
    %2486 = arith.andi %2336, %2485 : i1 loc(#loc182)
    %2487 = arith.andi %2482, %2312 : i1 loc(#loc182)
    %2488 = arith.andi %2336, %2487 : i1 loc(#loc182)
    %true_174 = arith.constant {acir.source_type = "bool"} true loc(#loc183)
    %false_175 = arith.constant {acir.source_type = "bool"} false loc(#loc184)
    %c0_i32_176 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc185)
    %2489 = arith.andi %2486, %2488 : i1 loc(#loc186)
    %2490 = acir.aggregate(%true_174, %false_175, %value_160, %2279, %c0_i32_176) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc186)
    %2491 = arith.xori %2336, %true : i1 loc(#loc187)
    %2492 = arith.andi %2484, %2489 : i1 loc(#loc187)
    %2493 = arith.ori %2491, %2492 : i1 loc(#loc187)
    %2494 = arith.andi %2481, %2493 : i1 loc(#loc187)
    %2495 = arith.andi %2336, %2484 : i1
    %2496 = arith.andi %2495, %2477 : i1
    %2497 = arith.xori %2339, %true : i1
    %2498 = arith.ori %2497, %2342 : i1
    %2499 = arith.ori %2339, %2336 : i1
    %2500 = arith.xori %2336, %true : i1
    %2501 = arith.ori %2500, %2452 : i1
    %2502 = arith.andi %2498, %2501 : i1
    %2503 = arith.xori %2324, %true : i1
    %2504 = arith.ori %2503, %2325 : i1
    %2505 = arith.andi %ready_161, %2312 : i1 loc(#loc190)
    acir.path %2324 {branch = 49 : i64, kind = "branch", txn = 6 : i64} loc(#loc190)
    acir.path %2324 {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc190)
    %2506 = arith.xori %2324, %true : i1 loc(#loc190)
    %2507 = arith.ori %2506, %2504 : i1 loc(#loc190)
    %2508 = arith.andi %2504, %ready_161 : i1 loc(#loc190)
    %2509 = arith.andi %2324, %2508 : i1 loc(#loc190)
    %2510 = arith.andi %2504, %2312 : i1 loc(#loc190)
    %2511 = arith.andi %2324, %2510 : i1 loc(#loc190)
    %2512 = arith.andi %2509, %2511 : i1 loc(#loc190)
    %2513 = arith.xori %2324, %true : i1 loc(#loc190)
    %2514 = arith.ori %2513, %2507 : i1 loc(#loc190)
    %2515 = arith.andi %2507, %2509 : i1 loc(#loc190)
    %2516 = arith.andi %2324, %2515 : i1 loc(#loc190)
    %2517 = arith.andi %2507, %2511 : i1 loc(#loc190)
    %2518 = arith.andi %2324, %2517 : i1 loc(#loc190)
    %value_177, %ready_178 = acir.read(%2324) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc179)
    %2519 = arith.xori %2324, %true : i1 loc(#loc179)
    %2520 = arith.andi %2514, %ready_178 : i1 loc(#loc179)
    %2521 = arith.ori %2519, %2520 : i1 loc(#loc179)
    %2522 = arith.andi %2494, %2521 : i1 loc(#loc179)
    %2523 = acir.extract %value_177 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc179)
    %2524 = acir.unary %2523 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc180)
    %2525 = arith.andi %2514, %ready_178 : i1 loc(#loc181)
    %2526 = arith.andi %2324, %2525 : i1 loc(#loc181)
    %2527 = arith.andi %2507, %2526 : i1 loc(#loc182)
    %2528 = arith.andi %2324, %2527 : i1 loc(#loc182)
    %2529 = arith.andi %2528, %2524 : i1 loc(#loc182)
    %2530 = arith.xori %2324, %true : i1 loc(#loc182)
    %2531 = arith.andi %2504, %2529 : i1 loc(#loc182)
    %2532 = arith.ori %2530, %2531 : i1 loc(#loc182)
    %2533 = arith.andi %2522, %2532 : i1 loc(#loc182)
    %2534 = arith.andi %2504, %2529 : i1 loc(#loc182)
    %2535 = arith.xori %2324, %true : i1 loc(#loc182)
    %2536 = arith.ori %2535, %2534 : i1 loc(#loc182)
    %2537 = arith.andi %2534, %ready_161 : i1 loc(#loc182)
    %2538 = arith.andi %2324, %2537 : i1 loc(#loc182)
    %2539 = arith.andi %2534, %2312 : i1 loc(#loc182)
    %2540 = arith.andi %2324, %2539 : i1 loc(#loc182)
    %true_179 = arith.constant {acir.source_type = "bool"} true loc(#loc183)
    %false_180 = arith.constant {acir.source_type = "bool"} false loc(#loc184)
    %c0_i32_181 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc185)
    %2541 = arith.andi %2538, %2540 : i1 loc(#loc186)
    %2542 = acir.aggregate(%true_179, %false_180, %value_160, %2279, %c0_i32_181) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc186)
    %2543 = arith.xori %2324, %true : i1 loc(#loc187)
    %2544 = arith.andi %2536, %2541 : i1 loc(#loc187)
    %2545 = arith.ori %2543, %2544 : i1 loc(#loc187)
    %2546 = arith.andi %2533, %2545 : i1 loc(#loc187)
    %2547 = arith.andi %2324, %2536 : i1
    %2548 = arith.andi %2547, %2529 : i1
    %2549 = arith.xori %2327, %true : i1
    %2550 = arith.ori %2549, %2330 : i1
    %2551 = arith.ori %2327, %2324 : i1
    %2552 = arith.xori %2324, %true : i1
    %2553 = arith.ori %2552, %2504 : i1
    %2554 = arith.andi %2550, %2553 : i1
    %2555 = arith.addi %value_160, %c1_i32_159 {acir.id = "v40", acir.source_type = "u32"} : i32 loc(#loc191)
    %2556 = arith.xori %1770, %true : i1 loc(#loc192)
    %2557 = arith.andi %2310, %ready_161 : i1 loc(#loc192)
    %2558 = arith.ori %2556, %2557 : i1 loc(#loc192)
    %2559 = arith.andi %2546, %2558 : i1 loc(#loc192)
    %2560 = arith.andi %2310, %ready_161 : i1 loc(#loc193)
    %2561 = arith.andi %1770, %2560 : i1 loc(#loc193)
    %2562 = arith.andi %2561, %2303 : i1 loc(#loc193)
    %2563 = acir.extract %value_105 {acir.id = "v11", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc194)
    %2564 = arith.andi %ready_106, %2562 : i1 loc(#loc195)
    acir.path %1770 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc195)
    %2565 = arith.xori %1770, %true : i1 loc(#loc195)
    %2566 = arith.ori %2565, %1776 : i1 loc(#loc195)
    %2567 = arith.andi %1776, %ready_106 : i1 loc(#loc195)
    %2568 = arith.andi %1770, %2567 : i1 loc(#loc195)
    %2569 = arith.andi %1776, %2562 : i1 loc(#loc195)
    %2570 = arith.andi %1770, %2569 : i1 loc(#loc195)
    %c0_i32_182 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc196)
    %c8_i32_183 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc197)
    %2571 = arith.cmpi ult, %2563, %c8_i32_183 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc198)
    %2572 = arith.xori %1770, %true : i1 loc(#loc199)
    %2573 = arith.andi %2566, %2568 : i1 loc(#loc199)
    %2574 = arith.ori %2572, %2573 : i1 loc(#loc199)
    %2575 = arith.andi %2559, %2574 : i1 loc(#loc199)
    %2576 = arith.cmpi ne, %2563, %c0_i32_182 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc200)
    %2577 = arith.xori %1770, %true : i1
    %2578 = arith.andi %2566, %2568 : i1
    %2579 = arith.ori %2577, %2578 : i1
    %2580 = arith.andi %2575, %2579 : i1
    %2581 = arith.andi %1770, %2576 : i1
    %2582 = arith.andi %2566, %2568 : i1
    %2583 = arith.xori %2576, %true : i1
    %2584 = arith.andi %1770, %2583 : i1
    %2585 = arith.andi %2566, %2568 : i1
    %2586 = arith.xori %2581, %true : i1
    %2587 = arith.ori %2586, %2582 : i1
    acir.path %2581 {branch = 50 : i64, kind = "branch", txn = 6 : i64} loc(#loc201)
    %value_184, %ready_185 = acir.read(%2581) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc201)
    %2588 = arith.xori %2581, %true : i1 loc(#loc201)
    %2589 = arith.andi %2587, %ready_185 : i1 loc(#loc201)
    %2590 = arith.ori %2588, %2589 : i1 loc(#loc201)
    %2591 = arith.andi %2580, %2590 : i1 loc(#loc201)
    %value_186, %ready_187 = acir.read(%2581) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc201)
    %2592 = arith.xori %2581, %true : i1 loc(#loc201)
    %2593 = arith.andi %2587, %ready_187 : i1 loc(#loc201)
    %2594 = arith.ori %2592, %2593 : i1 loc(#loc201)
    %2595 = arith.andi %2591, %2594 : i1 loc(#loc201)
    %2596 = arith.andi %ready_187, %2570 : i1 loc(#loc201)
    %2597 = arith.andi %2596, %2568 : i1 loc(#loc201)
    %2598 = acir.update %value_186, %value_160[%2563] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc201)
    %2599 = arith.xori %2581, %true : i1 loc(#loc201)
    %2600 = arith.andi %2587, %2597 : i1 loc(#loc201)
    %2601 = arith.ori %2599, %2600 : i1 loc(#loc201)
    %2602 = arith.andi %2595, %2601 : i1 loc(#loc201)
    %2603 = arith.xori %2584, %true : i1
    %2604 = arith.ori %2603, %2585 : i1
    %2605 = arith.ori %2584, %2581 : i1
    %2606 = arith.xori %2581, %true : i1
    %2607 = arith.ori %2606, %2587 : i1
    %2608 = arith.andi %2604, %2607 : i1
    %2609 = arith.andi %1770, %2566 : i1
    %2610 = acir.extract %value_105 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc202)
    %2611 = acir.extract %value_105 {acir.id = "v15", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc203)
    %2612 = arith.andi %ready_106, %2562 : i1 loc(#loc204)
    %2613 = arith.andi %2612, %ready_106 : i1 loc(#loc204)
    %2614 = arith.andi %2613, %2033 : i1 loc(#loc204)
    %2615 = arith.andi %2614, %2275 : i1 loc(#loc204)
    %2616 = acir.aggregate(%true_104, %2610, %value_160, %2611, %2036, %2278) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc204)
    acir.path %1770 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc205)
    %2617 = arith.xori %1770, %true : i1 loc(#loc205)
    %2618 = arith.ori %2617, %1776 : i1 loc(#loc205)
    %value_188, %ready_189 = acir.read(%1770) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    %2619 = arith.xori %1770, %true : i1 loc(#loc2)
    %2620 = arith.andi %2618, %ready_189 : i1 loc(#loc2)
    %2621 = arith.ori %2619, %2620 : i1 loc(#loc2)
    %2622 = arith.andi %2602, %2621 : i1 loc(#loc2)
    %2623 = arith.andi %2618, %ready_189 : i1 loc(#loc206)
    %2624 = arith.andi %1770, %2623 : i1 loc(#loc206)
    %2625 = acir.extract %value_188 {acir.id = "v18", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc205)
    %2626 = acir.unary %2625 "not" {acir.id = "v19", acir.source_type = "bool"} : i1 -> i1 loc(#loc207)
    %2627 = arith.xori %1770, %true : i1
    %2628 = arith.andi %1776, %2624 : i1
    %2629 = arith.ori %2627, %2628 : i1
    %2630 = arith.andi %2622, %2629 : i1
    %2631 = arith.andi %1770, %2626 : i1
    %2632 = arith.andi %1776, %2624 : i1
    %2633 = arith.xori %2626, %true : i1
    %2634 = arith.andi %1770, %2633 : i1
    %2635 = arith.andi %1776, %2624 : i1
    %2636 = arith.xori %2634, %true : i1
    %2637 = arith.ori %2636, %2635 : i1
    acir.path %2634 {branch = 51 : i64, kind = "branch", txn = 6 : i64} loc(#loc208)
    acir.path %2634 {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc208)
    %2638 = arith.xori %2634, %true : i1 loc(#loc208)
    %2639 = arith.ori %2638, %2637 : i1 loc(#loc208)
    %2640 = arith.andi %2637, %2615 : i1 loc(#loc208)
    %2641 = arith.andi %2634, %2640 : i1 loc(#loc208)
    %2642 = arith.xori %2634, %true : i1 loc(#loc208)
    %2643 = arith.ori %2642, %2639 : i1 loc(#loc208)
    %2644 = arith.andi %2639, %2641 : i1 loc(#loc208)
    %2645 = arith.andi %2634, %2644 : i1 loc(#loc208)
    %value_190, %ready_191 = acir.read(%2634) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc209)
    %2646 = arith.xori %2634, %true : i1 loc(#loc209)
    %2647 = arith.andi %2643, %ready_191 : i1 loc(#loc209)
    %2648 = arith.ori %2646, %2647 : i1 loc(#loc209)
    %2649 = arith.andi %2630, %2648 : i1 loc(#loc209)
    %2650 = acir.extract %value_190 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc209)
    %2651 = acir.unary %2650 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc210)
    %2652 = arith.andi %2643, %ready_191 : i1 loc(#loc211)
    %2653 = arith.andi %2634, %2652 : i1 loc(#loc211)
    %2654 = arith.andi %2639, %2653 : i1 loc(#loc212)
    %2655 = arith.andi %2634, %2654 : i1 loc(#loc212)
    %2656 = arith.andi %2655, %2651 : i1 loc(#loc212)
    %2657 = arith.xori %2634, %true : i1 loc(#loc212)
    %2658 = arith.andi %2637, %2656 : i1 loc(#loc212)
    %2659 = arith.ori %2657, %2658 : i1 loc(#loc212)
    %2660 = arith.andi %2649, %2659 : i1 loc(#loc212)
    %2661 = arith.andi %2637, %2656 : i1 loc(#loc212)
    %2662 = arith.xori %2634, %true : i1 loc(#loc212)
    %2663 = arith.ori %2662, %2661 : i1 loc(#loc212)
    %2664 = arith.andi %2661, %2615 : i1 loc(#loc212)
    %2665 = arith.andi %2634, %2664 : i1 loc(#loc212)
    %2666 = arith.xori %2634, %true : i1 loc(#loc213)
    %2667 = arith.andi %2663, %2665 : i1 loc(#loc213)
    %2668 = arith.ori %2666, %2667 : i1 loc(#loc213)
    %2669 = arith.andi %2660, %2668 : i1 loc(#loc213)
    %2670 = arith.andi %2634, %2663 : i1
    %2671 = arith.andi %2670, %2656 : i1
    %2672 = arith.xori %2631, %true : i1
    %2673 = arith.ori %2672, %2632 : i1
    acir.path %2631 {branch = 52 : i64, kind = "branch", txn = 6 : i64} loc(#loc214)
    acir.path %2631 {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc214)
    %2674 = arith.xori %2631, %true : i1 loc(#loc214)
    %2675 = arith.ori %2674, %2673 : i1 loc(#loc214)
    %2676 = arith.andi %2673, %2615 : i1 loc(#loc214)
    %2677 = arith.andi %2631, %2676 : i1 loc(#loc214)
    %2678 = arith.xori %2631, %true : i1 loc(#loc214)
    %2679 = arith.ori %2678, %2675 : i1 loc(#loc214)
    %2680 = arith.andi %2675, %2677 : i1 loc(#loc214)
    %2681 = arith.andi %2631, %2680 : i1 loc(#loc214)
    %value_192, %ready_193 = acir.read(%2631) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc209)
    %2682 = arith.xori %2631, %true : i1 loc(#loc209)
    %2683 = arith.andi %2679, %ready_193 : i1 loc(#loc209)
    %2684 = arith.ori %2682, %2683 : i1 loc(#loc209)
    %2685 = arith.andi %2669, %2684 : i1 loc(#loc209)
    %2686 = acir.extract %value_192 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc209)
    %2687 = acir.unary %2686 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc210)
    %2688 = arith.andi %2679, %ready_193 : i1 loc(#loc211)
    %2689 = arith.andi %2631, %2688 : i1 loc(#loc211)
    %2690 = arith.andi %2675, %2689 : i1 loc(#loc212)
    %2691 = arith.andi %2631, %2690 : i1 loc(#loc212)
    %2692 = arith.andi %2691, %2687 : i1 loc(#loc212)
    %2693 = arith.xori %2631, %true : i1 loc(#loc212)
    %2694 = arith.andi %2673, %2692 : i1 loc(#loc212)
    %2695 = arith.ori %2693, %2694 : i1 loc(#loc212)
    %2696 = arith.andi %2685, %2695 : i1 loc(#loc212)
    %2697 = arith.andi %2673, %2692 : i1 loc(#loc212)
    %2698 = arith.xori %2631, %true : i1 loc(#loc212)
    %2699 = arith.ori %2698, %2697 : i1 loc(#loc212)
    %2700 = arith.andi %2697, %2615 : i1 loc(#loc212)
    %2701 = arith.andi %2631, %2700 : i1 loc(#loc212)
    %2702 = arith.xori %2631, %true : i1 loc(#loc213)
    %2703 = arith.andi %2699, %2701 : i1 loc(#loc213)
    %2704 = arith.ori %2702, %2703 : i1 loc(#loc213)
    %2705 = arith.andi %2696, %2704 : i1 loc(#loc213)
    %2706 = arith.andi %2631, %2699 : i1
    %2707 = arith.andi %2706, %2692 : i1
    %2708 = arith.xori %2634, %true : i1
    %2709 = arith.ori %2708, %2637 : i1
    %2710 = arith.ori %2634, %2631 : i1
    %2711 = arith.xori %2631, %true : i1
    %2712 = arith.ori %2711, %2673 : i1
    %2713 = arith.andi %2709, %2712 : i1
    %2714 = arith.andi %1770, %1776 : i1
    %2715 = acir.fire "top.dispatch.dispatch#6" %2705 {successors = [], txn = 6 : i64}
    %2716 = arith.andi %2715, %1770 : i1
    acir.effect %2716() {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "pop", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> () loc(#loc122)
    %2717 = arith.andi %2715, %1770 : i1
    acir.effect %2717(%1800) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc127)
    %2718 = arith.andi %2715, %1770 : i1
    acir.effect %2718(%2042) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc127)
    %2719 = arith.andi %2715, %2351 : i1
    acir.effect %2719(%2392) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc187)
    %2720 = arith.andi %2715, %2348 : i1
    acir.effect %2720(%2438) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc187)
    %2721 = arith.andi %2715, %2336 : i1
    acir.effect %2721(%2490) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc187)
    %2722 = arith.andi %2715, %2324 : i1
    acir.effect %2722(%2542) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc187)
    %2723 = arith.andi %2715, %1770 : i1
    acir.effect %2723(%2555) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "write", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1, i32) -> () loc(#loc192)
    %2724 = arith.andi %2715, %1770 : i1
    acir.effect %2724(%2571) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc199)
    %2725 = arith.andi %2715, %2581 : i1
    acir.effect %2725(%2598) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "write", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1, !acir.array<i32, 8>) -> () loc(#loc201)
    %2726 = arith.andi %2715, %2634 : i1
    acir.effect %2726(%2616) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc213)
    %2727 = arith.andi %2715, %2631 : i1
    acir.effect %2727(%2616) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc213)
    %2728 = arith.xori %1771, %true : i1
    %2729 = arith.ori %2728, %1769 : i1
    %2730 = arith.ori %1771, %1770 : i1
    %2731 = arith.xori %1770, %true : i1
    %2732 = arith.ori %2731, %1773 : i1
    %2733 = arith.andi %2729, %2732 : i1
    acir.path %true {branch = 53 : i64, kind = "branch", txn = 7 : i64} loc(#loc215)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc215)
    %c1_i32_194 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc216)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc217)
    %c2_i32_195 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_196 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc218)
    %c3_i32_197 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc219)
    %c1_i32_198 = arith.constant {acir.source_type = "u32"} 1 : i32
    %value_199, %ready_200 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc220)
    %2734 = arith.subi %value_199, %c1_i32_198 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc221)
    %2735 = arith.andi %2734, %c3_i32_197 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc221)
    %2736 = arith.cmpi eq, %2735, %c0_i32_196 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc222)
    %2737 = arith.andi %ready_200, %ready_200 : i1
    %2738 = arith.xori %2736, %true : i1
    %2739 = arith.xori %2738, %true : i1
    %2740 = arith.ori %2739, %ready_200 : i1
    %2741 = arith.cmpi eq, %2735, %c1_i32_198 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc223)
    %2742 = arith.xori %2738, %true : i1
    %2743 = arith.andi %2740, %ready_200 : i1
    %2744 = arith.ori %2742, %2743 : i1
    %2745 = arith.andi %2737, %2744 : i1
    %2746 = arith.andi %2738, %2741 : i1
    %2747 = arith.andi %2740, %ready_200 : i1
    %2748 = arith.xori %2741, %true : i1
    %2749 = arith.andi %2738, %2748 : i1
    %2750 = arith.andi %2740, %ready_200 : i1
    %2751 = arith.xori %2749, %true : i1
    %2752 = arith.ori %2751, %2750 : i1
    %2753 = arith.cmpi eq, %2735, %c2_i32_195 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc224)
    %2754 = arith.xori %2749, %true : i1
    %2755 = arith.andi %2752, %ready_200 : i1
    %2756 = arith.ori %2754, %2755 : i1
    %2757 = arith.andi %2745, %2756 : i1
    %2758 = arith.andi %2749, %2753 : i1
    %2759 = arith.andi %2752, %ready_200 : i1
    %2760 = arith.xori %2753, %true : i1
    %2761 = arith.andi %2749, %2760 : i1
    %2762 = arith.andi %2752, %ready_200 : i1
    %2763 = arith.xori %2761, %true : i1
    %2764 = arith.ori %2763, %2762 : i1
    acir.path %2761 {branch = 54 : i64, kind = "branch", txn = 7 : i64} loc(#loc225)
    acir.path %2761 {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc225)
    %2765 = arith.xori %2761, %true : i1 loc(#loc225)
    %2766 = arith.ori %2765, %2764 : i1 loc(#loc225)
    %2767 = arith.andi %2764, %ready_200 : i1 loc(#loc225)
    %2768 = arith.andi %2761, %2767 : i1 loc(#loc225)
    %2769 = arith.xori %2761, %true : i1 loc(#loc225)
    %2770 = arith.ori %2769, %2766 : i1 loc(#loc225)
    %2771 = arith.andi %2766, %2768 : i1 loc(#loc225)
    %2772 = arith.andi %2761, %2771 : i1 loc(#loc225)
    %false_201 = arith.constant {acir.source_type = "bool"} false
    %value_202, %ready_203 = acir.read(%2761) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc226)
    %2773 = arith.xori %2761, %true : i1 loc(#loc226)
    %2774 = arith.andi %2770, %ready_203 : i1 loc(#loc226)
    %2775 = arith.ori %2773, %2774 : i1 loc(#loc226)
    %2776 = arith.andi %2757, %2775 : i1 loc(#loc226)
    %2777 = acir.extract %value_202 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc227)
    %2778 = arith.xori %2761, %true : i1
    %2779 = arith.andi %2770, %ready_203 : i1
    %2780 = arith.ori %2778, %2779 : i1
    %2781 = arith.andi %2776, %2780 : i1
    %2782 = arith.andi %2761, %2777 : i1
    %2783 = arith.andi %2770, %ready_203 : i1
    %2784 = arith.xori %2777, %true : i1
    %2785 = arith.andi %2761, %2784 : i1
    %2786 = arith.andi %2770, %ready_203 : i1
    %2787 = arith.xori %2785, %true : i1
    %2788 = arith.ori %2787, %2786 : i1
    %2789 = arith.xori %2782, %true : i1
    %2790 = arith.ori %2789, %2783 : i1
    %2791 = acir.extract %value_202 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc228)
    %2792 = arith.xori %2785, %true : i1
    %2793 = arith.ori %2792, %2788 : i1
    %2794 = arith.ori %2785, %2782 : i1
    %2795 = arith.xori %2782, %true : i1
    %2796 = arith.ori %2795, %2790 : i1
    %2797 = arith.andi %2793, %2796 : i1
    %2798 = arith.andi %2785, %2788 : i1
    %2799 = arith.andi %2790, %ready_203 : i1
    %2800 = arith.andi %2782, %2799 : i1
    %2801 = arith.ori %2798, %2800 : i1
    %2802 = arith.select %2785, %false_201, %2791 : i1
    %2803 = arith.xori %2761, %true : i1
    %2804 = arith.ori %2803, %2770 : i1
    %2805 = arith.xori %2761, %true : i1
    %2806 = arith.andi %2804, %2801 : i1
    %2807 = arith.ori %2805, %2806 : i1
    %2808 = arith.andi %2781, %2807 : i1
    %2809 = arith.andi %2761, %2802 : i1
    %2810 = arith.andi %2804, %2801 : i1
    %2811 = arith.xori %2802, %true : i1
    %2812 = arith.andi %2761, %2811 : i1
    %2813 = arith.andi %2804, %2801 : i1
    %2814 = arith.xori %2812, %true : i1
    %2815 = arith.ori %2814, %2813 : i1
    %2816 = arith.xori %2809, %true : i1
    %2817 = arith.ori %2816, %2810 : i1
    %2818 = acir.extract %value_202 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc229)
    %2819 = arith.andi %ready_203, %2772 : i1 loc(#loc229)
    %2820 = arith.cmpi eq, %2818, %value_199 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc229)
    %2821 = arith.xori %2812, %true : i1
    %2822 = arith.ori %2821, %2815 : i1
    %2823 = arith.ori %2812, %2809 : i1
    %2824 = arith.xori %2809, %true : i1
    %2825 = arith.ori %2824, %2817 : i1
    %2826 = arith.andi %2822, %2825 : i1
    %2827 = arith.andi %2812, %2815 : i1
    %2828 = arith.andi %2817, %2819 : i1
    %2829 = arith.andi %2809, %2828 : i1
    %2830 = arith.ori %2827, %2829 : i1
    %2831 = arith.select %2812, %false_201, %2820 : i1
    %2832 = arith.xori %2761, %true : i1
    %2833 = arith.ori %2832, %2804 : i1
    %2834 = arith.andi %2833, %2830 : i1 loc(#loc230)
    %2835 = arith.andi %2761, %2834 : i1 loc(#loc230)
    %2836 = arith.andi %2766, %2835 : i1 loc(#loc231)
    %2837 = arith.andi %2761, %2836 : i1 loc(#loc231)
    %2838 = arith.andi %2837, %2831 : i1 loc(#loc231)
    %2839 = arith.xori %2761, %true : i1 loc(#loc231)
    %2840 = arith.andi %2764, %2838 : i1 loc(#loc231)
    %2841 = arith.ori %2839, %2840 : i1 loc(#loc231)
    %2842 = arith.andi %2808, %2841 : i1 loc(#loc231)
    %2843 = arith.andi %2764, %2838 : i1 loc(#loc231)
    %2844 = arith.xori %2761, %true : i1 loc(#loc231)
    %2845 = arith.ori %2844, %2843 : i1 loc(#loc231)
    %2846 = arith.andi %2843, %ready_200 : i1 loc(#loc231)
    %2847 = arith.andi %2761, %2846 : i1 loc(#loc231)
    %c0_i32_204 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_205 = arith.constant {acir.source_type = "bool"} false
    %value_206, %ready_207 = acir.read(%2761) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc232)
    %2848 = arith.xori %2761, %true : i1 loc(#loc232)
    %2849 = arith.andi %2845, %ready_207 : i1 loc(#loc232)
    %2850 = arith.ori %2848, %2849 : i1 loc(#loc232)
    %2851 = arith.andi %2842, %2850 : i1 loc(#loc232)
    %2852 = acir.aggregate(%false_205, %false_205, %c0_i32_204, %c0_i32_204, %c0_i32_204) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc233)
    %2853 = arith.xori %2761, %true : i1 loc(#loc234)
    %2854 = arith.ori %2853, %2845 : i1 loc(#loc234)
    %2855 = arith.andi %2851, %2854 : i1 loc(#loc234)
    %2856 = arith.andi %2845, %ready_207 : i1 loc(#loc235)
    %2857 = arith.andi %2761, %2856 : i1 loc(#loc235)
    %2858 = arith.andi %2857, %2838 : i1 loc(#loc235)
    %2859 = arith.xori %2758, %true : i1
    %2860 = arith.ori %2859, %2759 : i1
    acir.path %2758 {branch = 55 : i64, kind = "branch", txn = 7 : i64} loc(#loc236)
    acir.path %2758 {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc236)
    %2861 = arith.xori %2758, %true : i1 loc(#loc236)
    %2862 = arith.ori %2861, %2860 : i1 loc(#loc236)
    %2863 = arith.andi %2860, %ready_200 : i1 loc(#loc236)
    %2864 = arith.andi %2758, %2863 : i1 loc(#loc236)
    %2865 = arith.xori %2758, %true : i1 loc(#loc236)
    %2866 = arith.ori %2865, %2862 : i1 loc(#loc236)
    %2867 = arith.andi %2862, %2864 : i1 loc(#loc236)
    %2868 = arith.andi %2758, %2867 : i1 loc(#loc236)
    %false_208 = arith.constant {acir.source_type = "bool"} false
    %value_209, %ready_210 = acir.read(%2758) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc226)
    %2869 = arith.xori %2758, %true : i1 loc(#loc226)
    %2870 = arith.andi %2866, %ready_210 : i1 loc(#loc226)
    %2871 = arith.ori %2869, %2870 : i1 loc(#loc226)
    %2872 = arith.andi %2855, %2871 : i1 loc(#loc226)
    %2873 = acir.extract %value_209 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc227)
    %2874 = arith.xori %2758, %true : i1
    %2875 = arith.andi %2866, %ready_210 : i1
    %2876 = arith.ori %2874, %2875 : i1
    %2877 = arith.andi %2872, %2876 : i1
    %2878 = arith.andi %2758, %2873 : i1
    %2879 = arith.andi %2866, %ready_210 : i1
    %2880 = arith.xori %2873, %true : i1
    %2881 = arith.andi %2758, %2880 : i1
    %2882 = arith.andi %2866, %ready_210 : i1
    %2883 = arith.xori %2881, %true : i1
    %2884 = arith.ori %2883, %2882 : i1
    %2885 = arith.xori %2878, %true : i1
    %2886 = arith.ori %2885, %2879 : i1
    %2887 = acir.extract %value_209 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc228)
    %2888 = arith.xori %2881, %true : i1
    %2889 = arith.ori %2888, %2884 : i1
    %2890 = arith.ori %2881, %2878 : i1
    %2891 = arith.xori %2878, %true : i1
    %2892 = arith.ori %2891, %2886 : i1
    %2893 = arith.andi %2889, %2892 : i1
    %2894 = arith.andi %2881, %2884 : i1
    %2895 = arith.andi %2886, %ready_210 : i1
    %2896 = arith.andi %2878, %2895 : i1
    %2897 = arith.ori %2894, %2896 : i1
    %2898 = arith.select %2881, %false_208, %2887 : i1
    %2899 = arith.xori %2758, %true : i1
    %2900 = arith.ori %2899, %2866 : i1
    %2901 = arith.xori %2758, %true : i1
    %2902 = arith.andi %2900, %2897 : i1
    %2903 = arith.ori %2901, %2902 : i1
    %2904 = arith.andi %2877, %2903 : i1
    %2905 = arith.andi %2758, %2898 : i1
    %2906 = arith.andi %2900, %2897 : i1
    %2907 = arith.xori %2898, %true : i1
    %2908 = arith.andi %2758, %2907 : i1
    %2909 = arith.andi %2900, %2897 : i1
    %2910 = arith.xori %2908, %true : i1
    %2911 = arith.ori %2910, %2909 : i1
    %2912 = arith.xori %2905, %true : i1
    %2913 = arith.ori %2912, %2906 : i1
    %2914 = acir.extract %value_209 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc229)
    %2915 = arith.andi %ready_210, %2868 : i1 loc(#loc229)
    %2916 = arith.cmpi eq, %2914, %value_199 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc229)
    %2917 = arith.xori %2908, %true : i1
    %2918 = arith.ori %2917, %2911 : i1
    %2919 = arith.ori %2908, %2905 : i1
    %2920 = arith.xori %2905, %true : i1
    %2921 = arith.ori %2920, %2913 : i1
    %2922 = arith.andi %2918, %2921 : i1
    %2923 = arith.andi %2908, %2911 : i1
    %2924 = arith.andi %2913, %2915 : i1
    %2925 = arith.andi %2905, %2924 : i1
    %2926 = arith.ori %2923, %2925 : i1
    %2927 = arith.select %2908, %false_208, %2916 : i1
    %2928 = arith.xori %2758, %true : i1
    %2929 = arith.ori %2928, %2900 : i1
    %2930 = arith.andi %2929, %2926 : i1 loc(#loc230)
    %2931 = arith.andi %2758, %2930 : i1 loc(#loc230)
    %2932 = arith.andi %2862, %2931 : i1 loc(#loc231)
    %2933 = arith.andi %2758, %2932 : i1 loc(#loc231)
    %2934 = arith.andi %2933, %2927 : i1 loc(#loc231)
    %2935 = arith.xori %2758, %true : i1 loc(#loc231)
    %2936 = arith.andi %2860, %2934 : i1 loc(#loc231)
    %2937 = arith.ori %2935, %2936 : i1 loc(#loc231)
    %2938 = arith.andi %2904, %2937 : i1 loc(#loc231)
    %2939 = arith.andi %2860, %2934 : i1 loc(#loc231)
    %2940 = arith.xori %2758, %true : i1 loc(#loc231)
    %2941 = arith.ori %2940, %2939 : i1 loc(#loc231)
    %2942 = arith.andi %2939, %ready_200 : i1 loc(#loc231)
    %2943 = arith.andi %2758, %2942 : i1 loc(#loc231)
    %c0_i32_211 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_212 = arith.constant {acir.source_type = "bool"} false
    %value_213, %ready_214 = acir.read(%2758) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc232)
    %2944 = arith.xori %2758, %true : i1 loc(#loc232)
    %2945 = arith.andi %2941, %ready_214 : i1 loc(#loc232)
    %2946 = arith.ori %2944, %2945 : i1 loc(#loc232)
    %2947 = arith.andi %2938, %2946 : i1 loc(#loc232)
    %2948 = acir.aggregate(%false_212, %false_212, %c0_i32_211, %c0_i32_211, %c0_i32_211) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc233)
    %2949 = arith.xori %2758, %true : i1 loc(#loc234)
    %2950 = arith.ori %2949, %2941 : i1 loc(#loc234)
    %2951 = arith.andi %2947, %2950 : i1 loc(#loc234)
    %2952 = arith.andi %2941, %ready_214 : i1 loc(#loc235)
    %2953 = arith.andi %2758, %2952 : i1 loc(#loc235)
    %2954 = arith.andi %2953, %2934 : i1 loc(#loc235)
    %2955 = arith.xori %2761, %true : i1
    %2956 = arith.ori %2955, %2764 : i1
    %2957 = arith.ori %2761, %2758 : i1
    %2958 = arith.xori %2758, %true : i1
    %2959 = arith.ori %2958, %2860 : i1
    %2960 = arith.andi %2956, %2959 : i1
    %2961 = arith.andi %2764, %2858 : i1
    %2962 = arith.andi %2761, %2961 : i1
    %2963 = arith.andi %2860, %2954 : i1
    %2964 = arith.andi %2758, %2963 : i1
    %2965 = arith.ori %2962, %2964 : i1
    %2966 = arith.select %2761, %value_206, %value_213 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2967 = arith.xori %2749, %true : i1
    %2968 = arith.ori %2967, %2752 : i1
    %2969 = arith.xori %2746, %true : i1
    %2970 = arith.ori %2969, %2747 : i1
    acir.path %2746 {branch = 56 : i64, kind = "branch", txn = 7 : i64} loc(#loc237)
    acir.path %2746 {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc237)
    %2971 = arith.xori %2746, %true : i1 loc(#loc237)
    %2972 = arith.ori %2971, %2970 : i1 loc(#loc237)
    %2973 = arith.andi %2970, %ready_200 : i1 loc(#loc237)
    %2974 = arith.andi %2746, %2973 : i1 loc(#loc237)
    %2975 = arith.xori %2746, %true : i1 loc(#loc237)
    %2976 = arith.ori %2975, %2972 : i1 loc(#loc237)
    %2977 = arith.andi %2972, %2974 : i1 loc(#loc237)
    %2978 = arith.andi %2746, %2977 : i1 loc(#loc237)
    %false_215 = arith.constant {acir.source_type = "bool"} false
    %value_216, %ready_217 = acir.read(%2746) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc226)
    %2979 = arith.xori %2746, %true : i1 loc(#loc226)
    %2980 = arith.andi %2976, %ready_217 : i1 loc(#loc226)
    %2981 = arith.ori %2979, %2980 : i1 loc(#loc226)
    %2982 = arith.andi %2951, %2981 : i1 loc(#loc226)
    %2983 = acir.extract %value_216 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc227)
    %2984 = arith.xori %2746, %true : i1
    %2985 = arith.andi %2976, %ready_217 : i1
    %2986 = arith.ori %2984, %2985 : i1
    %2987 = arith.andi %2982, %2986 : i1
    %2988 = arith.andi %2746, %2983 : i1
    %2989 = arith.andi %2976, %ready_217 : i1
    %2990 = arith.xori %2983, %true : i1
    %2991 = arith.andi %2746, %2990 : i1
    %2992 = arith.andi %2976, %ready_217 : i1
    %2993 = arith.xori %2991, %true : i1
    %2994 = arith.ori %2993, %2992 : i1
    %2995 = arith.xori %2988, %true : i1
    %2996 = arith.ori %2995, %2989 : i1
    %2997 = acir.extract %value_216 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc228)
    %2998 = arith.xori %2991, %true : i1
    %2999 = arith.ori %2998, %2994 : i1
    %3000 = arith.ori %2991, %2988 : i1
    %3001 = arith.xori %2988, %true : i1
    %3002 = arith.ori %3001, %2996 : i1
    %3003 = arith.andi %2999, %3002 : i1
    %3004 = arith.andi %2991, %2994 : i1
    %3005 = arith.andi %2996, %ready_217 : i1
    %3006 = arith.andi %2988, %3005 : i1
    %3007 = arith.ori %3004, %3006 : i1
    %3008 = arith.select %2991, %false_215, %2997 : i1
    %3009 = arith.xori %2746, %true : i1
    %3010 = arith.ori %3009, %2976 : i1
    %3011 = arith.xori %2746, %true : i1
    %3012 = arith.andi %3010, %3007 : i1
    %3013 = arith.ori %3011, %3012 : i1
    %3014 = arith.andi %2987, %3013 : i1
    %3015 = arith.andi %2746, %3008 : i1
    %3016 = arith.andi %3010, %3007 : i1
    %3017 = arith.xori %3008, %true : i1
    %3018 = arith.andi %2746, %3017 : i1
    %3019 = arith.andi %3010, %3007 : i1
    %3020 = arith.xori %3018, %true : i1
    %3021 = arith.ori %3020, %3019 : i1
    %3022 = arith.xori %3015, %true : i1
    %3023 = arith.ori %3022, %3016 : i1
    %3024 = acir.extract %value_216 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc229)
    %3025 = arith.andi %ready_217, %2978 : i1 loc(#loc229)
    %3026 = arith.cmpi eq, %3024, %value_199 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc229)
    %3027 = arith.xori %3018, %true : i1
    %3028 = arith.ori %3027, %3021 : i1
    %3029 = arith.ori %3018, %3015 : i1
    %3030 = arith.xori %3015, %true : i1
    %3031 = arith.ori %3030, %3023 : i1
    %3032 = arith.andi %3028, %3031 : i1
    %3033 = arith.andi %3018, %3021 : i1
    %3034 = arith.andi %3023, %3025 : i1
    %3035 = arith.andi %3015, %3034 : i1
    %3036 = arith.ori %3033, %3035 : i1
    %3037 = arith.select %3018, %false_215, %3026 : i1
    %3038 = arith.xori %2746, %true : i1
    %3039 = arith.ori %3038, %3010 : i1
    %3040 = arith.andi %3039, %3036 : i1 loc(#loc230)
    %3041 = arith.andi %2746, %3040 : i1 loc(#loc230)
    %3042 = arith.andi %2972, %3041 : i1 loc(#loc231)
    %3043 = arith.andi %2746, %3042 : i1 loc(#loc231)
    %3044 = arith.andi %3043, %3037 : i1 loc(#loc231)
    %3045 = arith.xori %2746, %true : i1 loc(#loc231)
    %3046 = arith.andi %2970, %3044 : i1 loc(#loc231)
    %3047 = arith.ori %3045, %3046 : i1 loc(#loc231)
    %3048 = arith.andi %3014, %3047 : i1 loc(#loc231)
    %3049 = arith.andi %2970, %3044 : i1 loc(#loc231)
    %3050 = arith.xori %2746, %true : i1 loc(#loc231)
    %3051 = arith.ori %3050, %3049 : i1 loc(#loc231)
    %3052 = arith.andi %3049, %ready_200 : i1 loc(#loc231)
    %3053 = arith.andi %2746, %3052 : i1 loc(#loc231)
    %c0_i32_218 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_219 = arith.constant {acir.source_type = "bool"} false
    %value_220, %ready_221 = acir.read(%2746) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc232)
    %3054 = arith.xori %2746, %true : i1 loc(#loc232)
    %3055 = arith.andi %3051, %ready_221 : i1 loc(#loc232)
    %3056 = arith.ori %3054, %3055 : i1 loc(#loc232)
    %3057 = arith.andi %3048, %3056 : i1 loc(#loc232)
    %3058 = acir.aggregate(%false_219, %false_219, %c0_i32_218, %c0_i32_218, %c0_i32_218) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc233)
    %3059 = arith.xori %2746, %true : i1 loc(#loc234)
    %3060 = arith.ori %3059, %3051 : i1 loc(#loc234)
    %3061 = arith.andi %3057, %3060 : i1 loc(#loc234)
    %3062 = arith.andi %3051, %ready_221 : i1 loc(#loc235)
    %3063 = arith.andi %2746, %3062 : i1 loc(#loc235)
    %3064 = arith.andi %3063, %3044 : i1 loc(#loc235)
    %3065 = arith.xori %2749, %true : i1
    %3066 = arith.ori %3065, %2968 : i1
    %3067 = arith.ori %2749, %2746 : i1
    %3068 = arith.xori %2746, %true : i1
    %3069 = arith.ori %3068, %2970 : i1
    %3070 = arith.andi %3066, %3069 : i1
    %3071 = arith.andi %2968, %2965 : i1
    %3072 = arith.andi %2749, %3071 : i1
    %3073 = arith.andi %2970, %3064 : i1
    %3074 = arith.andi %2746, %3073 : i1
    %3075 = arith.ori %3072, %3074 : i1
    %3076 = arith.select %2749, %2966, %value_220 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3077 = arith.xori %2738, %true : i1
    %3078 = arith.ori %3077, %2740 : i1
    %3079 = arith.xori %2736, %true : i1
    %3080 = arith.ori %3079, %ready_200 : i1
    acir.path %2736 {branch = 57 : i64, kind = "branch", txn = 7 : i64} loc(#loc238)
    acir.path %2736 {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc238)
    %3081 = arith.xori %2736, %true : i1 loc(#loc238)
    %3082 = arith.ori %3081, %3080 : i1 loc(#loc238)
    %3083 = arith.andi %3080, %ready_200 : i1 loc(#loc238)
    %3084 = arith.andi %2736, %3083 : i1 loc(#loc238)
    %3085 = arith.xori %2736, %true : i1 loc(#loc238)
    %3086 = arith.ori %3085, %3082 : i1 loc(#loc238)
    %3087 = arith.andi %3082, %3084 : i1 loc(#loc238)
    %3088 = arith.andi %2736, %3087 : i1 loc(#loc238)
    %false_222 = arith.constant {acir.source_type = "bool"} false
    %value_223, %ready_224 = acir.read(%2736) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc226)
    %3089 = arith.xori %2736, %true : i1 loc(#loc226)
    %3090 = arith.andi %3086, %ready_224 : i1 loc(#loc226)
    %3091 = arith.ori %3089, %3090 : i1 loc(#loc226)
    %3092 = arith.andi %3061, %3091 : i1 loc(#loc226)
    %3093 = acir.extract %value_223 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc227)
    %3094 = arith.xori %2736, %true : i1
    %3095 = arith.andi %3086, %ready_224 : i1
    %3096 = arith.ori %3094, %3095 : i1
    %3097 = arith.andi %3092, %3096 : i1
    %3098 = arith.andi %2736, %3093 : i1
    %3099 = arith.andi %3086, %ready_224 : i1
    %3100 = arith.xori %3093, %true : i1
    %3101 = arith.andi %2736, %3100 : i1
    %3102 = arith.andi %3086, %ready_224 : i1
    %3103 = arith.xori %3101, %true : i1
    %3104 = arith.ori %3103, %3102 : i1
    %3105 = arith.xori %3098, %true : i1
    %3106 = arith.ori %3105, %3099 : i1
    %3107 = acir.extract %value_223 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc228)
    %3108 = arith.xori %3101, %true : i1
    %3109 = arith.ori %3108, %3104 : i1
    %3110 = arith.ori %3101, %3098 : i1
    %3111 = arith.xori %3098, %true : i1
    %3112 = arith.ori %3111, %3106 : i1
    %3113 = arith.andi %3109, %3112 : i1
    %3114 = arith.andi %3101, %3104 : i1
    %3115 = arith.andi %3106, %ready_224 : i1
    %3116 = arith.andi %3098, %3115 : i1
    %3117 = arith.ori %3114, %3116 : i1
    %3118 = arith.select %3101, %false_222, %3107 : i1
    %3119 = arith.xori %2736, %true : i1
    %3120 = arith.ori %3119, %3086 : i1
    %3121 = arith.xori %2736, %true : i1
    %3122 = arith.andi %3120, %3117 : i1
    %3123 = arith.ori %3121, %3122 : i1
    %3124 = arith.andi %3097, %3123 : i1
    %3125 = arith.andi %2736, %3118 : i1
    %3126 = arith.andi %3120, %3117 : i1
    %3127 = arith.xori %3118, %true : i1
    %3128 = arith.andi %2736, %3127 : i1
    %3129 = arith.andi %3120, %3117 : i1
    %3130 = arith.xori %3128, %true : i1
    %3131 = arith.ori %3130, %3129 : i1
    %3132 = arith.xori %3125, %true : i1
    %3133 = arith.ori %3132, %3126 : i1
    %3134 = acir.extract %value_223 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc229)
    %3135 = arith.andi %ready_224, %3088 : i1 loc(#loc229)
    %3136 = arith.cmpi eq, %3134, %value_199 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc229)
    %3137 = arith.xori %3128, %true : i1
    %3138 = arith.ori %3137, %3131 : i1
    %3139 = arith.ori %3128, %3125 : i1
    %3140 = arith.xori %3125, %true : i1
    %3141 = arith.ori %3140, %3133 : i1
    %3142 = arith.andi %3138, %3141 : i1
    %3143 = arith.andi %3128, %3131 : i1
    %3144 = arith.andi %3133, %3135 : i1
    %3145 = arith.andi %3125, %3144 : i1
    %3146 = arith.ori %3143, %3145 : i1
    %3147 = arith.select %3128, %false_222, %3136 : i1
    %3148 = arith.xori %2736, %true : i1
    %3149 = arith.ori %3148, %3120 : i1
    %3150 = arith.andi %3149, %3146 : i1 loc(#loc230)
    %3151 = arith.andi %2736, %3150 : i1 loc(#loc230)
    %3152 = arith.andi %3082, %3151 : i1 loc(#loc231)
    %3153 = arith.andi %2736, %3152 : i1 loc(#loc231)
    %3154 = arith.andi %3153, %3147 : i1 loc(#loc231)
    %3155 = arith.xori %2736, %true : i1 loc(#loc231)
    %3156 = arith.andi %3080, %3154 : i1 loc(#loc231)
    %3157 = arith.ori %3155, %3156 : i1 loc(#loc231)
    %3158 = arith.andi %3124, %3157 : i1 loc(#loc231)
    %3159 = arith.andi %3080, %3154 : i1 loc(#loc231)
    %3160 = arith.xori %2736, %true : i1 loc(#loc231)
    %3161 = arith.ori %3160, %3159 : i1 loc(#loc231)
    %3162 = arith.andi %3159, %ready_200 : i1 loc(#loc231)
    %3163 = arith.andi %2736, %3162 : i1 loc(#loc231)
    %c0_i32_225 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_226 = arith.constant {acir.source_type = "bool"} false
    %value_227, %ready_228 = acir.read(%2736) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc232)
    %3164 = arith.xori %2736, %true : i1 loc(#loc232)
    %3165 = arith.andi %3161, %ready_228 : i1 loc(#loc232)
    %3166 = arith.ori %3164, %3165 : i1 loc(#loc232)
    %3167 = arith.andi %3158, %3166 : i1 loc(#loc232)
    %3168 = acir.aggregate(%false_226, %false_226, %c0_i32_225, %c0_i32_225, %c0_i32_225) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc233)
    %3169 = arith.xori %2736, %true : i1 loc(#loc234)
    %3170 = arith.ori %3169, %3161 : i1 loc(#loc234)
    %3171 = arith.andi %3167, %3170 : i1 loc(#loc234)
    %3172 = arith.andi %3161, %ready_228 : i1 loc(#loc235)
    %3173 = arith.andi %2736, %3172 : i1 loc(#loc235)
    %3174 = arith.andi %3173, %3154 : i1 loc(#loc235)
    %3175 = arith.xori %2738, %true : i1
    %3176 = arith.ori %3175, %3078 : i1
    %3177 = arith.ori %2738, %2736 : i1
    %3178 = arith.xori %2736, %true : i1
    %3179 = arith.ori %3178, %3080 : i1
    %3180 = arith.andi %3176, %3179 : i1
    %3181 = arith.andi %3078, %3075 : i1
    %3182 = arith.andi %2738, %3181 : i1
    %3183 = arith.andi %3080, %3174 : i1
    %3184 = arith.andi %2736, %3183 : i1
    %3185 = arith.ori %3182, %3184 : i1
    %3186 = arith.select %2738, %3076, %value_227 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3187 = arith.addi %value_199, %c1_i32_198 {acir.id = "v38", acir.source_type = "u32"} : i32 loc(#loc239)
    %3188 = arith.andi %3171, %ready_200 : i1 loc(#loc240)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc241)
    %c0_i32_229 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc242)
    %3189 = acir.extract %3186 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc243)
    %3190 = arith.cmpi ne, %3189, %c0_i32_229 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc243)
    %3191 = arith.andi %3188, %3185 : i1
    %3192 = arith.xori %3190, %true : i1
    %3193 = arith.xori %3190, %true : i1
    %3194 = arith.ori %3193, %3185 : i1
    %3195 = acir.extract %3186 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc244)
    acir.path %3190 {branch = 58 : i64, kind = "branch", txn = 7 : i64} loc(#loc245)
    %value_230, %ready_231 = acir.read(%3190) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc245)
    %3196 = arith.xori %3190, %true : i1 loc(#loc245)
    %3197 = arith.andi %3194, %ready_231 : i1 loc(#loc245)
    %3198 = arith.ori %3196, %3197 : i1 loc(#loc245)
    %3199 = arith.andi %3191, %3198 : i1 loc(#loc245)
    %value_232, %ready_233 = acir.read(%3190) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc245)
    %3200 = arith.xori %3190, %true : i1 loc(#loc245)
    %3201 = arith.andi %3194, %ready_233 : i1 loc(#loc245)
    %3202 = arith.ori %3200, %3201 : i1 loc(#loc245)
    %3203 = arith.andi %3199, %3202 : i1 loc(#loc245)
    %3204 = acir.extract %3186 {acir.id = "v17", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc246)
    %3205 = arith.andi %ready_233, %3185 : i1 loc(#loc245)
    %3206 = arith.andi %3205, %3185 : i1 loc(#loc245)
    %3207 = acir.update %value_232, %3195[%3204] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc245)
    %3208 = arith.xori %3190, %true : i1 loc(#loc245)
    %3209 = arith.andi %3194, %3206 : i1 loc(#loc245)
    %3210 = arith.ori %3208, %3209 : i1 loc(#loc245)
    %3211 = arith.andi %3203, %3210 : i1 loc(#loc245)
    %3212 = acir.extract %3186 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc247)
    %value_234, %ready_235 = acir.read(%3190) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc248)
    %3213 = arith.xori %3190, %true : i1 loc(#loc248)
    %3214 = arith.andi %3194, %ready_235 : i1 loc(#loc248)
    %3215 = arith.ori %3213, %3214 : i1 loc(#loc248)
    %3216 = arith.andi %3211, %3215 : i1 loc(#loc248)
    %value_236, %ready_237 = acir.read(%3190) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc248)
    %3217 = arith.xori %3190, %true : i1 loc(#loc248)
    %3218 = arith.andi %3194, %ready_237 : i1 loc(#loc248)
    %3219 = arith.ori %3217, %3218 : i1 loc(#loc248)
    %3220 = arith.andi %3216, %3219 : i1 loc(#loc248)
    %3221 = acir.extract %3186 {acir.id = "v30", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc249)
    %3222 = arith.andi %ready_237, %3185 : i1 loc(#loc248)
    %3223 = arith.andi %3222, %3185 : i1 loc(#loc248)
    %3224 = acir.update %value_236, %3212[%3221] {acir.id = "v31", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc248)
    %3225 = arith.xori %3190, %true : i1 loc(#loc248)
    %3226 = arith.andi %3194, %3223 : i1 loc(#loc248)
    %3227 = arith.ori %3225, %3226 : i1 loc(#loc248)
    %3228 = arith.andi %3220, %3227 : i1 loc(#loc248)
    %3229 = arith.xori %3192, %true : i1
    %3230 = arith.ori %3229, %3185 : i1
    %3231 = arith.ori %3192, %3190 : i1
    %3232 = arith.xori %3190, %true : i1
    %3233 = arith.ori %3232, %3194 : i1
    %3234 = arith.andi %3230, %3233 : i1
    %value_238, %ready_239 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit"], method = "read", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc250)
    %3235 = arith.andi %3228, %ready_239 : i1 loc(#loc250)
    %3236 = arith.addi %value_238, %c1_i32_194 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc250)
    %3237 = arith.andi %3235, %ready_239 : i1 loc(#loc251)
    %3238 = acir.extract %3186 {acir.id = "v11", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc252)
    %3239 = arith.andi %3237, %3185 : i1 loc(#loc253)
    %3240 = acir.fire "top.commit.commit#7" %3239 {successors = [], txn = 7 : i64}
    %3241 = arith.andi %3240, %2761 : i1
    acir.effect %3241(%2852) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc234)
    %3242 = arith.andi %3240, %2758 : i1
    acir.effect %3242(%2948) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc234)
    %3243 = arith.andi %3240, %2746 : i1
    acir.effect %3243(%3058) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc234)
    %3244 = arith.andi %3240, %2736 : i1
    acir.effect %3244(%3168) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc234)
    acir.effect %3240(%3187) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "write", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1, i32) -> () loc(#loc240)
    %3245 = arith.andi %3240, %3190 : i1
    acir.effect %3245(%3207) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1, !acir.array<i32, 8>) -> () loc(#loc245)
    %3246 = arith.andi %3240, %3190 : i1
    acir.effect %3246(%3224) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1, !acir.array<i32, 8>) -> () loc(#loc248)
    acir.effect %3240(%3236) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1, i32) -> () loc(#loc251)
    acir.effect %3240(%3238) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.last_tag", txn = 7 : i64} : (i1, i32) -> () loc(#loc253)
    acir.path %true {branch = 59 : i64, kind = "branch", txn = 8 : i64} loc(#loc254)
    acir.path %true {branch = 59 : i64, frames = ["top.audit:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 8 : i64} loc(#loc254)
    %value_240, %ready_241 = acir.read(%true) {branch = 59 : i64, frames = ["top.audit:observe"], method = "get", port = 0 : i64, resource = "top.event", txn = 8 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc255)
    %3247 = acir.extract %value_240 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc256)
    %3248 = arith.andi %ready_241, %ready_241 : i1 loc(#loc257)
    %3249 = acir.fire "top.audit.observe#8" %3248 {successors = [], txn = 8 : i64}
    acir.effect %3249(%3247) {branch = 59 : i64, frames = ["top.audit:observe"], method = "write", port = 0 : i64, resource = "top.audit.last_tag", txn = 8 : i64} : (i1, i32) -> () loc(#loc257)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:13)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":44:16)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:13)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:19)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:31)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:47)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:19)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:29)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:38)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:51)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:59)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:38)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:51)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:59)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:17)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:16)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:17)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:16)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:32)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:55)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:75)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:9)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:20)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:16)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:17)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:9)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:9)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:9)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:20)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:13)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:45)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:16)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:9)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:20)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:13)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":45:17)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:27)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:38)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:48)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:66)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:16)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:9)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:16)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":79:13)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:18)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:21)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:19)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:9)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:9)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:9)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:20)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:25)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:12)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":70:17)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:9)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:27)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:38)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:48)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:66)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:16)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:9)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:25)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:13)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":69:13)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:13)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:16)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:22)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:21)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:27)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:17)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:12)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:14)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:14)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:13)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:17)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:12)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:33)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:29)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:50)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":63:13)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:13)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:13)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:13)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":131:13)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:13)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":30:17)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:12)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:20)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:16)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:40)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:29)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:17)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:20)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:16)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:40)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:29)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:17)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":40:13)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":49:9)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:23)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:24)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:16)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:9)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:12)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":165:15)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":166:19)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":166:12)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":168:17)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":117:21)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:27)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:17)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":117:12)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:14)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:14)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:16)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":69:16)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":69:9)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:9)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:20)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:13)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:20)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:13)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:20)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:13)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:12)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:28)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:44)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:16)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:9)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:39)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:20)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:13)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:39)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:20)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:13)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":164:20)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":164:13)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:30)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:16)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:23)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:9)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:20)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:21)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:27)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:15)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:17)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:12)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:14)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:14)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:13)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":53:20)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":53:16)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":53:9)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":55:20)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:26)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:32)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:48)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:17)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:9)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:13)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:13)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:13)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":111:16)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":111:9)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:9)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:18)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:21)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:16)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:9)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:12)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":177:13)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":44:9)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:20)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:16)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:9)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":14:20)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":16:9)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":70:9)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:29)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":64:17)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:21)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:27)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":139:15)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:17)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:12)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:14)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:14)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:21)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:17)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:16)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:32)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:48)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:9)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":75:20)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:17)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":78:17)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":78:9)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":79:9)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:21)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:21)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:21)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:16)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:9)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:9)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:24)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:12)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:35)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:13)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:23)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:35)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:13)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:23)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:19)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:20)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:9)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":80:17)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:20)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:9)

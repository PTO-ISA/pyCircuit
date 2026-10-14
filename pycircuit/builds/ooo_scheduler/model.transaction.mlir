module attributes {acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, format_version = 9 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default, name = "lhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "done", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}, acir.stage = "transaction"} {
  %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
  %c0_i64 = arith.constant 0 : i64
  %c8_i64 = arith.constant 8 : i64
  %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
  %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
  %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
  %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
  %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 5 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.event", source_branch = 29 : i64, target_branches = [59], to = 8 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}], schedule = [3, 1, 5, 2, 0, 4, 6, 7, 8], stage = "transaction"} {
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
    acir.state @top.rf.committed : !acir.array<i32, [8]> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rf.registers : !acir.array<i32, [8]> {initial = [0 : i32, 0 : i32, 6 : i32, 7 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rf.renamed : !acir.array<i32, [8]> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.right.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]], kind = "cell"}
    acir.state @top.rob.head : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.rob.slots.0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slots.1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slots.2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slots.3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.tail : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity = 4 : i64, initial = [[1 : i32, 1 : i32, 2 : i32, 3 : i32], [0 : i32, 4 : i32, 1 : i32, 2 : i32], [0 : i32, 5 : i32, 2 : i32, 3 : i32], [0 : i32, 6 : i32, 4 : i32, 5 : i32]], kind = "queue"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    acir.path %true {branch = 0 : i64, frames = ["top.left:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.left:peek"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    %0 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    %1 = acir.assert %0
    %2 = arith.andi %ready, %1 : i1
    acir.path %true {branch = 0 : i64, frames = ["top.right:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc3)
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = ["top.right:peek"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    %3 = acir.assert %0
    %4 = arith.andi %ready_1, %3 : i1
    %5 = acir.extract %value {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc4)
    %6 = arith.xori %5, %true : i1
    %7 = arith.ori %5, %2 : i1
    %8 = arith.ori %6, %2 : i1
    %9 = acir.extract %value {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc5)
    %10 = acir.extract %9 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc5)
    %11 = arith.andi %6, %7 : i1
    %12 = arith.andi %8, %2 : i1
    %13 = arith.andi %5, %12 : i1
    %14 = arith.ori %11, %13 : i1
    %15 = arith.select %5, %10, %false : i1
    %16 = arith.xori %15, %true : i1
    %17 = arith.ori %15, %14 : i1
    %18 = arith.ori %16, %14 : i1
    %19 = acir.extract %value {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc6)
    %20 = acir.extract %19 {acir.id = "v12", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc6)
    %21 = arith.andi %16, %17 : i1
    %22 = arith.andi %18, %2 : i1
    %23 = arith.andi %15, %22 : i1
    %24 = arith.ori %21, %23 : i1
    %25 = arith.select %15, %20, %false : i1
    %26 = acir.extract %value_0 {acir.id = "v17", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc7)
    %27 = arith.xori %26, %true : i1
    %28 = arith.ori %26, %4 : i1
    %29 = arith.ori %27, %4 : i1
    %30 = acir.extract %value_0 {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc8)
    %31 = acir.extract %30 {acir.id = "v19", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc8)
    %32 = arith.andi %27, %28 : i1
    %33 = arith.andi %29, %4 : i1
    %34 = arith.andi %26, %33 : i1
    %35 = arith.ori %32, %34 : i1
    %36 = arith.select %26, %31, %false : i1
    %37 = arith.xori %36, %true : i1
    %38 = arith.ori %36, %35 : i1
    %39 = arith.ori %37, %35 : i1
    %40 = acir.extract %value_0 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc9)
    %41 = acir.extract %40 {acir.id = "v25", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc9)
    %42 = arith.andi %37, %38 : i1
    %43 = arith.andi %39, %4 : i1
    %44 = arith.andi %36, %43 : i1
    %45 = arith.ori %42, %44 : i1
    %46 = arith.select %36, %41, %false : i1
    %47 = arith.xori %25, %true : i1
    %48 = arith.ori %25, %24 : i1
    %49 = arith.ori %47, %24 : i1
    %50 = acir.extract %value {acir.id = "v30", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc10)
    %51 = arith.cmpi eq, %50, %c0_i32 {acir.id = "v32", acir.source_type = "bool"} : i32 loc(#loc10)
    %52 = arith.andi %47, %48 : i1
    %53 = arith.andi %49, %2 : i1
    %54 = arith.andi %25, %53 : i1
    %55 = arith.ori %52, %54 : i1
    %56 = arith.select %25, %51, %false : i1
    %57 = arith.xori %46, %true : i1
    %58 = arith.ori %46, %45 : i1
    %59 = arith.ori %57, %45 : i1
    %60 = acir.extract %value_0 {acir.id = "v37", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc11)
    %61 = arith.cmpi eq, %60, %c0_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc11)
    %62 = arith.andi %57, %58 : i1
    %63 = arith.andi %59, %4 : i1
    %64 = arith.andi %46, %63 : i1
    %65 = arith.ori %62, %64 : i1
    %66 = arith.select %46, %61, %false : i1
    %67 = acir.extract %value {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc12)
    %68 = arith.cmpi eq, %67, %c1_i32 {acir.id = "v46", acir.source_type = "bool"} : i32 loc(#loc12)
    %69 = arith.select %25, %68, %false : i1
    %70 = acir.extract %value_0 {acir.id = "v51", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc13)
    %71 = arith.cmpi eq, %70, %c1_i32 {acir.id = "v53", acir.source_type = "bool"} : i32 loc(#loc13)
    %72 = arith.select %46, %71, %false : i1
    %73 = arith.xori %56, %true : i1
    %74 = arith.ori %56, %55 : i1
    %75 = arith.ori %73, %55 : i1
    %76 = acir.unary %66 "not" {acir.id = "v58", acir.source_type = "bool"} : i1 -> i1 loc(#loc14)
    %77 = arith.andi %56, %76 : i1
    %78 = arith.andi %75, %65 : i1
    %79 = arith.xori %76, %true : i1
    %80 = arith.andi %56, %79 : i1
    %81 = arith.xori %80, %true : i1
    %82 = arith.ori %81, %78 : i1
    %83 = acir.extract %value {acir.id = "v60", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc15)
    %84 = acir.extract %value_0 {acir.id = "v61", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc16)
    %85 = arith.andi %2, %4 : i1 loc(#loc15)
    %86 = arith.cmpi ult, %83, %84 {acir.id = "v62", acir.source_type = "bool"} : i32 loc(#loc15)
    %87 = arith.xori %77, %true : i1
    %88 = arith.ori %87, %78 : i1
    %89 = arith.andi %82, %85 : i1
    %90 = arith.andi %80, %89 : i1
    %91 = arith.andi %77, %88 : i1
    %92 = arith.ori %90, %91 : i1
    %93 = arith.select %80, %86, %true : i1
    %94 = arith.ori %73, %75 : i1
    %95 = arith.andi %73, %74 : i1
    %96 = arith.andi %94, %92 : i1
    %97 = arith.andi %56, %96 : i1
    %98 = arith.ori %95, %97 : i1
    %99 = arith.select %56, %93, %false : i1
    %100 = arith.xori %69, %true : i1
    %101 = arith.ori %69, %55 : i1
    %102 = arith.ori %100, %55 : i1
    %103 = acir.unary %72 "not" {acir.id = "v70", acir.source_type = "bool"} : i1 -> i1 loc(#loc17)
    %104 = arith.andi %69, %103 : i1
    %105 = arith.andi %102, %65 : i1
    %106 = arith.xori %103, %true : i1
    %107 = arith.andi %69, %106 : i1
    %108 = arith.xori %107, %true : i1
    %109 = arith.ori %108, %105 : i1
    %110 = acir.extract %value {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc18)
    %111 = acir.extract %value_0 {acir.id = "v73", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc19)
    %112 = arith.cmpi ult, %110, %111 {acir.id = "v74", acir.source_type = "bool"} : i32 loc(#loc18)
    %113 = arith.xori %104, %true : i1
    %114 = arith.ori %113, %105 : i1
    %115 = arith.andi %109, %85 : i1
    %116 = arith.andi %107, %115 : i1
    %117 = arith.andi %104, %114 : i1
    %118 = arith.ori %116, %117 : i1
    %119 = arith.select %107, %112, %true : i1
    %120 = arith.ori %100, %102 : i1
    %121 = arith.andi %100, %101 : i1
    %122 = arith.andi %120, %118 : i1
    %123 = arith.andi %69, %122 : i1
    %124 = arith.ori %121, %123 : i1
    %125 = arith.select %69, %119, %false : i1
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc20)
    acir.path %true {branch = 1 : i64, frames = ["top.scheduler:issue_add"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc20)
    %126 = arith.xori %99, %true : i1 loc(#loc21)
    %127 = arith.ori %99, %98 : i1
    %128 = arith.ori %126, %98 : i1
    acir.path %99 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc21)
    acir.path %99 {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc21)
    %129 = arith.ori %126, %128 : i1 loc(#loc21)
    %130 = arith.andi %99, %128 : i1 loc(#loc21)
    acir.path %99 {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.left:take", "top.left:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc21)
    %131 = arith.ori %126, %129 : i1 loc(#loc21)
    %132 = arith.andi %129, %130 : i1 loc(#loc21)
    %133 = arith.andi %99, %132 : i1 loc(#loc21)
    %value_2, %ready_3 = acir.read(%99) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.left:take", "top.left:issuable"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %134 = arith.andi %131, %ready_3 : i1 loc(#loc22)
    %135 = arith.ori %126, %134 : i1 loc(#loc22)
    %136 = arith.andi %98, %135 : i1 loc(#loc22)
    %137 = acir.extract %value_2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %138 = arith.andi %136, %135 : i1
    %139 = arith.andi %99, %137 : i1
    %140 = arith.xori %137, %true : i1
    %141 = arith.andi %99, %140 : i1
    %142 = arith.xori %141, %true : i1
    %143 = arith.ori %142, %134 : i1
    %144 = arith.xori %139, %true : i1
    %145 = arith.ori %144, %134 : i1
    %146 = acir.extract %value_2 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %147 = arith.andi %ready_3, %133 : i1 loc(#loc24)
    %148 = arith.cmpi eq, %146, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc24)
    %149 = arith.andi %141, %143 : i1
    %150 = arith.andi %145, %147 : i1
    %151 = arith.andi %139, %150 : i1
    %152 = arith.ori %149, %151 : i1
    %153 = arith.select %141, %false, %148 : i1
    %154 = arith.ori %126, %131 : i1
    %155 = arith.andi %154, %152 : i1
    %156 = arith.ori %126, %155 : i1
    %157 = arith.andi %138, %156 : i1
    %158 = arith.andi %99, %153 : i1
    %159 = arith.xori %153, %true : i1
    %160 = arith.andi %99, %159 : i1
    %161 = arith.xori %160, %true : i1
    %162 = arith.ori %161, %155 : i1
    %163 = arith.xori %158, %true : i1
    %164 = arith.ori %163, %155 : i1
    %165 = acir.extract %value_2 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %166 = acir.extract %165 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %167 = arith.andi %160, %162 : i1
    %168 = arith.andi %164, %ready_3 : i1
    %169 = arith.andi %158, %168 : i1
    %170 = arith.ori %167, %169 : i1
    %171 = arith.select %160, %false, %166 : i1
    %172 = arith.ori %126, %154 : i1
    %173 = arith.andi %172, %170 : i1
    %174 = arith.ori %126, %173 : i1
    %175 = arith.andi %157, %174 : i1
    %176 = arith.andi %99, %171 : i1
    %177 = arith.xori %171, %true : i1
    %178 = arith.andi %99, %177 : i1
    %179 = arith.xori %178, %true : i1
    %180 = arith.ori %179, %173 : i1
    %181 = arith.xori %176, %true : i1
    %182 = arith.ori %181, %173 : i1
    %183 = acir.extract %value_2 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %184 = acir.extract %183 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %185 = arith.andi %178, %180 : i1
    %186 = arith.andi %182, %ready_3 : i1
    %187 = arith.andi %176, %186 : i1
    %188 = arith.ori %185, %187 : i1
    %189 = arith.select %178, %false, %184 : i1
    %190 = arith.ori %126, %172 : i1
    %191 = arith.ori %126, %190 : i1
    %192 = arith.andi %175, %191 : i1
    %193 = arith.andi %190, %188 : i1
    %194 = arith.andi %99, %193 : i1
    %195 = arith.andi %129, %194 : i1 loc(#loc27)
    %196 = arith.andi %99, %195 : i1 loc(#loc27)
    %197 = arith.andi %196, %189 : i1 loc(#loc27)
    %198 = arith.andi %128, %197 : i1 loc(#loc27)
    %199 = arith.ori %126, %198 : i1 loc(#loc27)
    %200 = arith.andi %192, %199 : i1 loc(#loc27)
    %value_4, %ready_5 = acir.read(%99) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc28)
    %201 = arith.andi %199, %ready_5 : i1 loc(#loc28)
    %202 = arith.ori %126, %201 : i1 loc(#loc28)
    %203 = arith.andi %200, %202 : i1 loc(#loc28)
    %204 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %205 = acir.aggregate() {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %206 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %204, %205) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc29)
    %207 = arith.ori %126, %199 : i1 loc(#loc30)
    %208 = arith.andi %203, %207 : i1 loc(#loc30)
    %209 = arith.andi %208, %207 : i1
    %210 = arith.andi %99, %201 : i1
    %211 = arith.andi %210, %197 : i1
    %212 = arith.andi %126, %127 : i1
    %213 = arith.ori %212, %130 : i1
    %214 = arith.andi %128, %211 : i1
    %215 = arith.andi %99, %214 : i1
    %216 = arith.ori %212, %215 : i1
    %217 = arith.andi %209, %213 : i1
    %218 = arith.ori %126, %213 : i1
    %219 = arith.ori %99, %213 : i1
    acir.path %126 {branch = 3 : i64, kind = "branch", txn = 0 : i64} loc(#loc31)
    acir.path %126 {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc31)
    %220 = arith.ori %99, %219 : i1 loc(#loc31)
    %221 = arith.andi %126, %219 : i1 loc(#loc31)
    acir.path %126 {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.right:take", "top.right:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc31)
    %222 = arith.ori %99, %220 : i1 loc(#loc31)
    %223 = arith.andi %220, %221 : i1 loc(#loc31)
    %224 = arith.andi %126, %223 : i1 loc(#loc31)
    %value_6, %ready_7 = acir.read(%126) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.right:take", "top.right:issuable"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %225 = arith.andi %222, %ready_7 : i1 loc(#loc22)
    %226 = arith.ori %99, %225 : i1 loc(#loc22)
    %227 = arith.andi %217, %226 : i1 loc(#loc22)
    %228 = acir.extract %value_6 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %229 = arith.andi %227, %226 : i1
    %230 = arith.andi %126, %228 : i1
    %231 = arith.xori %228, %true : i1
    %232 = arith.andi %126, %231 : i1
    %233 = arith.xori %232, %true : i1
    %234 = arith.ori %233, %225 : i1
    %235 = arith.xori %230, %true : i1
    %236 = arith.ori %235, %225 : i1
    %237 = acir.extract %value_6 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %238 = arith.andi %ready_7, %224 : i1 loc(#loc24)
    %239 = arith.cmpi eq, %237, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc24)
    %240 = arith.andi %232, %234 : i1
    %241 = arith.andi %236, %238 : i1
    %242 = arith.andi %230, %241 : i1
    %243 = arith.ori %240, %242 : i1
    %244 = arith.select %232, %false, %239 : i1
    %245 = arith.ori %99, %222 : i1
    %246 = arith.andi %245, %243 : i1
    %247 = arith.ori %99, %246 : i1
    %248 = arith.andi %229, %247 : i1
    %249 = arith.andi %126, %244 : i1
    %250 = arith.xori %244, %true : i1
    %251 = arith.andi %126, %250 : i1
    %252 = arith.xori %251, %true : i1
    %253 = arith.ori %252, %246 : i1
    %254 = arith.xori %249, %true : i1
    %255 = arith.ori %254, %246 : i1
    %256 = acir.extract %value_6 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %257 = acir.extract %256 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %258 = arith.andi %251, %253 : i1
    %259 = arith.andi %255, %ready_7 : i1
    %260 = arith.andi %249, %259 : i1
    %261 = arith.ori %258, %260 : i1
    %262 = arith.select %251, %false, %257 : i1
    %263 = arith.ori %99, %245 : i1
    %264 = arith.andi %263, %261 : i1
    %265 = arith.ori %99, %264 : i1
    %266 = arith.andi %248, %265 : i1
    %267 = arith.andi %126, %262 : i1
    %268 = arith.xori %262, %true : i1
    %269 = arith.andi %126, %268 : i1
    %270 = arith.xori %269, %true : i1
    %271 = arith.ori %270, %264 : i1
    %272 = arith.xori %267, %true : i1
    %273 = arith.ori %272, %264 : i1
    %274 = acir.extract %value_6 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %275 = acir.extract %274 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %276 = arith.andi %269, %271 : i1
    %277 = arith.andi %273, %ready_7 : i1
    %278 = arith.andi %267, %277 : i1
    %279 = arith.ori %276, %278 : i1
    %280 = arith.select %269, %false, %275 : i1
    %281 = arith.ori %99, %263 : i1
    %282 = arith.ori %99, %281 : i1
    %283 = arith.andi %266, %282 : i1
    %284 = arith.andi %281, %279 : i1
    %285 = arith.andi %126, %284 : i1
    %286 = arith.andi %220, %285 : i1 loc(#loc27)
    %287 = arith.andi %126, %286 : i1 loc(#loc27)
    %288 = arith.andi %287, %280 : i1 loc(#loc27)
    %289 = arith.andi %219, %288 : i1 loc(#loc27)
    %290 = arith.ori %99, %289 : i1 loc(#loc27)
    %291 = arith.andi %283, %290 : i1 loc(#loc27)
    %value_8, %ready_9 = acir.read(%126) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc28)
    %292 = arith.andi %290, %ready_9 : i1 loc(#loc28)
    %293 = arith.ori %99, %292 : i1 loc(#loc28)
    %294 = arith.andi %291, %293 : i1 loc(#loc28)
    %295 = arith.ori %99, %290 : i1 loc(#loc30)
    %296 = arith.andi %294, %295 : i1 loc(#loc30)
    %297 = arith.andi %296, %295 : i1
    %298 = arith.andi %126, %292 : i1
    %299 = arith.andi %298, %288 : i1
    %300 = arith.andi %218, %213 : i1
    %301 = arith.andi %99, %300 : i1
    %302 = arith.ori %301, %221 : i1
    %303 = arith.select %99, %99, %true : i1
    %304 = arith.andi %218, %216 : i1
    %305 = arith.andi %99, %304 : i1
    %306 = arith.andi %219, %299 : i1
    %307 = arith.andi %126, %306 : i1
    %308 = arith.ori %305, %307 : i1
    %309 = arith.select %99, %value_4, %value_8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %310 = arith.andi %308, %302 : i1
    %311 = arith.andi %310, %303 : i1
    acir.path %311 {branch = 4 : i64, kind = "branch", txn = 0 : i64}
    %value_10, %ready_11 = acir.read(%311) {branch = 4 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.add_requests", txn = 0 : i64} : (i1) -> (i1, i1)
    %312 = arith.xori %311, %true : i1
    %313 = arith.ori %312, %ready_11 : i1
    %314 = arith.andi %297, %313 : i1
    %315 = arith.xori %value_10, %true : i1
    %316 = arith.ori %312, %315 : i1
    %317 = arith.andi %314, %316 : i1
    %318 = acir.fire "top.scheduler.issue_add#0" %317 {successors = [], txn = 0 : i64}
    %319 = arith.andi %318, %99 : i1
    acir.effect %319(%0) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.left:take", "top.left:issuable"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
    acir.effect %319(%206) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc30)
    acir.effect %319(%0) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
    %320 = arith.andi %318, %126 : i1
    acir.effect %320(%0) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.right:take", "top.right:issuable"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
    acir.effect %320(%206) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc30)
    acir.effect %320(%0) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
    %321 = arith.andi %318, %311 : i1
    acir.effect %321(%309) {branch = 4 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.add_requests", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
    acir.path %true {branch = 5 : i64, kind = "branch", txn = 1 : i64} loc(#loc32)
    acir.path %true {branch = 5 : i64, frames = ["top.scheduler:issue_mul"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc32)
    %322 = arith.xori %125, %true : i1 loc(#loc33)
    %323 = arith.ori %125, %124 : i1
    %324 = arith.ori %322, %124 : i1
    acir.path %125 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc33)
    acir.path %125 {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc33)
    %325 = arith.ori %322, %324 : i1 loc(#loc33)
    %326 = arith.andi %125, %324 : i1 loc(#loc33)
    acir.path %125 {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.left:take", "top.left:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc21)
    %327 = arith.ori %322, %325 : i1 loc(#loc21)
    %328 = arith.andi %325, %326 : i1 loc(#loc21)
    %329 = arith.andi %125, %328 : i1 loc(#loc21)
    %value_12, %ready_13 = acir.read(%125) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.left:take", "top.left:issuable"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %330 = arith.andi %327, %ready_13 : i1 loc(#loc22)
    %331 = arith.ori %322, %330 : i1 loc(#loc22)
    %332 = arith.andi %124, %331 : i1 loc(#loc22)
    %333 = acir.extract %value_12 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %334 = arith.andi %332, %331 : i1
    %335 = arith.andi %125, %333 : i1
    %336 = arith.xori %333, %true : i1
    %337 = arith.andi %125, %336 : i1
    %338 = arith.xori %337, %true : i1
    %339 = arith.ori %338, %330 : i1
    %340 = arith.xori %335, %true : i1
    %341 = arith.ori %340, %330 : i1
    %342 = acir.extract %value_12 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %343 = arith.andi %ready_13, %329 : i1 loc(#loc24)
    %344 = arith.cmpi eq, %342, %c1_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc24)
    %345 = arith.andi %337, %339 : i1
    %346 = arith.andi %341, %343 : i1
    %347 = arith.andi %335, %346 : i1
    %348 = arith.ori %345, %347 : i1
    %349 = arith.select %337, %false, %344 : i1
    %350 = arith.ori %322, %327 : i1
    %351 = arith.andi %350, %348 : i1
    %352 = arith.ori %322, %351 : i1
    %353 = arith.andi %334, %352 : i1
    %354 = arith.andi %125, %349 : i1
    %355 = arith.xori %349, %true : i1
    %356 = arith.andi %125, %355 : i1
    %357 = arith.xori %356, %true : i1
    %358 = arith.ori %357, %351 : i1
    %359 = arith.xori %354, %true : i1
    %360 = arith.ori %359, %351 : i1
    %361 = acir.extract %value_12 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %362 = acir.extract %361 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %363 = arith.andi %356, %358 : i1
    %364 = arith.andi %360, %ready_13 : i1
    %365 = arith.andi %354, %364 : i1
    %366 = arith.ori %363, %365 : i1
    %367 = arith.select %356, %false, %362 : i1
    %368 = arith.ori %322, %350 : i1
    %369 = arith.andi %368, %366 : i1
    %370 = arith.ori %322, %369 : i1
    %371 = arith.andi %353, %370 : i1
    %372 = arith.andi %125, %367 : i1
    %373 = arith.xori %367, %true : i1
    %374 = arith.andi %125, %373 : i1
    %375 = arith.xori %374, %true : i1
    %376 = arith.ori %375, %369 : i1
    %377 = arith.xori %372, %true : i1
    %378 = arith.ori %377, %369 : i1
    %379 = acir.extract %value_12 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %380 = acir.extract %379 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %381 = arith.andi %374, %376 : i1
    %382 = arith.andi %378, %ready_13 : i1
    %383 = arith.andi %372, %382 : i1
    %384 = arith.ori %381, %383 : i1
    %385 = arith.select %374, %false, %380 : i1
    %386 = arith.ori %322, %368 : i1
    %387 = arith.ori %322, %386 : i1
    %388 = arith.andi %371, %387 : i1
    %389 = arith.andi %386, %384 : i1
    %390 = arith.andi %125, %389 : i1
    %391 = arith.andi %325, %390 : i1 loc(#loc27)
    %392 = arith.andi %125, %391 : i1 loc(#loc27)
    %393 = arith.andi %392, %385 : i1 loc(#loc27)
    %394 = arith.andi %324, %393 : i1 loc(#loc27)
    %395 = arith.ori %322, %394 : i1 loc(#loc27)
    %396 = arith.andi %388, %395 : i1 loc(#loc27)
    %value_14, %ready_15 = acir.read(%125) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc28)
    %397 = arith.andi %395, %ready_15 : i1 loc(#loc28)
    %398 = arith.ori %322, %397 : i1 loc(#loc28)
    %399 = arith.andi %396, %398 : i1 loc(#loc28)
    %400 = arith.ori %322, %395 : i1 loc(#loc30)
    %401 = arith.andi %399, %400 : i1 loc(#loc30)
    %402 = arith.andi %401, %400 : i1
    %403 = arith.andi %125, %397 : i1
    %404 = arith.andi %403, %393 : i1
    %405 = arith.andi %322, %323 : i1
    %406 = arith.ori %405, %326 : i1
    %407 = arith.andi %324, %404 : i1
    %408 = arith.andi %125, %407 : i1
    %409 = arith.ori %405, %408 : i1
    %410 = arith.andi %402, %406 : i1
    %411 = arith.ori %322, %406 : i1
    %412 = arith.ori %125, %406 : i1
    acir.path %322 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc34)
    acir.path %322 {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc34)
    %413 = arith.ori %125, %412 : i1 loc(#loc34)
    %414 = arith.andi %322, %412 : i1 loc(#loc34)
    acir.path %322 {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.right:take", "top.right:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc31)
    %415 = arith.ori %125, %413 : i1 loc(#loc31)
    %416 = arith.andi %413, %414 : i1 loc(#loc31)
    %417 = arith.andi %322, %416 : i1 loc(#loc31)
    %value_16, %ready_17 = acir.read(%322) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.right:take", "top.right:issuable"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc22)
    %418 = arith.andi %415, %ready_17 : i1 loc(#loc22)
    %419 = arith.ori %125, %418 : i1 loc(#loc22)
    %420 = arith.andi %410, %419 : i1 loc(#loc22)
    %421 = acir.extract %value_16 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc23)
    %422 = arith.andi %420, %419 : i1
    %423 = arith.andi %322, %421 : i1
    %424 = arith.xori %421, %true : i1
    %425 = arith.andi %322, %424 : i1
    %426 = arith.xori %425, %true : i1
    %427 = arith.ori %426, %418 : i1
    %428 = arith.xori %423, %true : i1
    %429 = arith.ori %428, %418 : i1
    %430 = acir.extract %value_16 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %431 = arith.andi %ready_17, %417 : i1 loc(#loc24)
    %432 = arith.cmpi eq, %430, %c1_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc24)
    %433 = arith.andi %425, %427 : i1
    %434 = arith.andi %429, %431 : i1
    %435 = arith.andi %423, %434 : i1
    %436 = arith.ori %433, %435 : i1
    %437 = arith.select %425, %false, %432 : i1
    %438 = arith.ori %125, %415 : i1
    %439 = arith.andi %438, %436 : i1
    %440 = arith.ori %125, %439 : i1
    %441 = arith.andi %422, %440 : i1
    %442 = arith.andi %322, %437 : i1
    %443 = arith.xori %437, %true : i1
    %444 = arith.andi %322, %443 : i1
    %445 = arith.xori %444, %true : i1
    %446 = arith.ori %445, %439 : i1
    %447 = arith.xori %442, %true : i1
    %448 = arith.ori %447, %439 : i1
    %449 = acir.extract %value_16 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc25)
    %450 = acir.extract %449 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc25)
    %451 = arith.andi %444, %446 : i1
    %452 = arith.andi %448, %ready_17 : i1
    %453 = arith.andi %442, %452 : i1
    %454 = arith.ori %451, %453 : i1
    %455 = arith.select %444, %false, %450 : i1
    %456 = arith.ori %125, %438 : i1
    %457 = arith.andi %456, %454 : i1
    %458 = arith.ori %125, %457 : i1
    %459 = arith.andi %441, %458 : i1
    %460 = arith.andi %322, %455 : i1
    %461 = arith.xori %455, %true : i1
    %462 = arith.andi %322, %461 : i1
    %463 = arith.xori %462, %true : i1
    %464 = arith.ori %463, %457 : i1
    %465 = arith.xori %460, %true : i1
    %466 = arith.ori %465, %457 : i1
    %467 = acir.extract %value_16 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
    %468 = acir.extract %467 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
    %469 = arith.andi %462, %464 : i1
    %470 = arith.andi %466, %ready_17 : i1
    %471 = arith.andi %460, %470 : i1
    %472 = arith.ori %469, %471 : i1
    %473 = arith.select %462, %false, %468 : i1
    %474 = arith.ori %125, %456 : i1
    %475 = arith.ori %125, %474 : i1
    %476 = arith.andi %459, %475 : i1
    %477 = arith.andi %474, %472 : i1
    %478 = arith.andi %322, %477 : i1
    %479 = arith.andi %413, %478 : i1 loc(#loc27)
    %480 = arith.andi %322, %479 : i1 loc(#loc27)
    %481 = arith.andi %480, %473 : i1 loc(#loc27)
    %482 = arith.andi %412, %481 : i1 loc(#loc27)
    %483 = arith.ori %125, %482 : i1 loc(#loc27)
    %484 = arith.andi %476, %483 : i1 loc(#loc27)
    %value_18, %ready_19 = acir.read(%322) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc28)
    %485 = arith.andi %483, %ready_19 : i1 loc(#loc28)
    %486 = arith.ori %125, %485 : i1 loc(#loc28)
    %487 = arith.andi %484, %486 : i1 loc(#loc28)
    %488 = arith.ori %125, %483 : i1 loc(#loc30)
    %489 = arith.andi %487, %488 : i1 loc(#loc30)
    %490 = arith.andi %489, %488 : i1
    %491 = arith.andi %322, %485 : i1
    %492 = arith.andi %491, %481 : i1
    %493 = arith.andi %411, %406 : i1
    %494 = arith.andi %125, %493 : i1
    %495 = arith.ori %494, %414 : i1
    %496 = arith.select %125, %125, %true : i1
    %497 = arith.andi %411, %409 : i1
    %498 = arith.andi %125, %497 : i1
    %499 = arith.andi %412, %492 : i1
    %500 = arith.andi %322, %499 : i1
    %501 = arith.ori %498, %500 : i1
    %502 = arith.select %125, %value_14, %value_18 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %503 = arith.andi %501, %495 : i1
    %504 = arith.andi %503, %496 : i1
    acir.path %504 {branch = 8 : i64, kind = "branch", txn = 1 : i64}
    %value_20, %ready_21 = acir.read(%504) {branch = 8 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mul_requests", txn = 1 : i64} : (i1) -> (i1, i1)
    %505 = arith.xori %504, %true : i1
    %506 = arith.ori %505, %ready_21 : i1
    %507 = arith.andi %490, %506 : i1
    %508 = arith.xori %value_20, %true : i1
    %509 = arith.ori %505, %508 : i1
    %510 = arith.andi %507, %509 : i1
    %511 = acir.fire "top.scheduler.issue_mul#1" %510 {successors = [], txn = 1 : i64}
    %512 = arith.andi %511, %125 : i1
    acir.effect %512(%0) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.left:take", "top.left:issuable"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    acir.effect %512(%206) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc30)
    acir.effect %512(%0) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    %513 = arith.andi %511, %322 : i1
    acir.effect %513(%0) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.right:take", "top.right:issuable"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    acir.effect %513(%206) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc30)
    acir.effect %513(%0) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    %514 = arith.andi %511, %504 : i1
    acir.effect %514(%502) {branch = 8 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mul_requests", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
    acir.path %true {branch = 9 : i64, kind = "branch", txn = 2 : i64} loc(#loc35)
    acir.path %true {branch = 9 : i64, frames = ["top.adder:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc35)
    %value_22, %ready_23 = acir.read(%true) {branch = 9 : i64, frames = ["top.adder:execute"], method = "peek", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc36)
    %value_24, %ready_25 = acir.read(%true) {branch = 9 : i64, frames = ["top.adder:execute"], method = "empty", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc36)
    %515 = arith.andi %ready_23, %ready_25 : i1 loc(#loc36)
    %516 = arith.xori %value_24, %true : i1 loc(#loc36)
    %517 = arith.andi %515, %516 : i1 loc(#loc36)
    %518 = acir.extract %value_22 {acir.id = "v8", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc37)
    %519 = acir.extract %value_22 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc38)
    %520 = acir.extract %value_22 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc39)
    %521 = acir.extract %520 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc39)
    %522 = acir.extract %value_22 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc40)
    %523 = acir.extract %522 {acir.id = "v13", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc40)
    %524 = arith.addi %521, %523 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc39)
    %525 = acir.aggregate(%518, %519, %524) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc41)
    acir.path %ready_23 {branch = 10 : i64, kind = "branch", txn = 2 : i64}
    %value_26, %ready_27 = acir.read(%ready_23) {branch = 10 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.add_done", txn = 2 : i64} : (i1) -> (i1, i1)
    %526 = arith.xori %ready_23, %true : i1
    %527 = arith.ori %526, %ready_27 : i1
    %528 = arith.andi %517, %527 : i1
    %529 = arith.xori %value_26, %true : i1
    %530 = arith.ori %526, %529 : i1
    %531 = arith.andi %528, %530 : i1
    %532 = acir.fire "top.adder.execute#2" %531 {successors = [0], txn = 2 : i64}
    acir.effect %532() {branch = 9 : i64, frames = ["top.adder:execute"], method = "pop", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> () loc(#loc36)
    %533 = arith.andi %532, %ready_23 : i1
    acir.effect %533(%525) {branch = 10 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.add_done", txn = 2 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> ()
    %value_28, %ready_29 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.multiplier.active", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc42)
    %534 = arith.xori %value_28, %true : i1 loc(#loc43)
    %535 = arith.ori %value_28, %ready_29 : i1
    %536 = arith.andi %534, %535 : i1 loc(#loc44)
    acir.path %534 {branch = 11 : i64, kind = "branch", txn = 3 : i64} loc(#loc44)
    acir.path %534 {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc44)
    %537 = arith.ori %value_28, %535 : i1 loc(#loc44)
    %value_30, %ready_31 = acir.read(%534) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "peek", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc45)
    %538 = arith.andi %537, %ready_31 : i1 loc(#loc45)
    %539 = arith.ori %value_28, %538 : i1 loc(#loc45)
    %540 = arith.andi %536, %539 : i1 loc(#loc45)
    %541 = arith.andi %540, %539 : i1 loc(#loc45)
    %value_32, %ready_33 = acir.read(%534) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "empty", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc45)
    %542 = arith.andi %537, %ready_33 : i1 loc(#loc45)
    %543 = arith.ori %value_28, %542 : i1 loc(#loc45)
    %544 = arith.andi %541, %543 : i1 loc(#loc45)
    %545 = arith.xori %value_32, %true : i1 loc(#loc45)
    %546 = arith.andi %537, %545 : i1 loc(#loc45)
    %547 = arith.ori %value_28, %546 : i1 loc(#loc45)
    %548 = arith.andi %544, %547 : i1 loc(#loc45)
    %549 = arith.andi %548, %539 : i1 loc(#loc46)
    %550 = arith.ori %value_28, %537 : i1 loc(#loc47)
    %551 = arith.andi %549, %550 : i1 loc(#loc47)
    %552 = arith.andi %551, %550 : i1 loc(#loc48)
    %553 = acir.fire "top.multiplier.accept#3" %552 {successors = [1], txn = 3 : i64}
    %554 = arith.andi %553, %534 : i1
    acir.effect %554() {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "pop", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> () loc(#loc45)
    acir.effect %554(%value_30) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.request", txn = 3 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc46)
    acir.effect %554(%c2_i32) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 3 : i64} : (i1, i32) -> () loc(#loc47)
    acir.effect %554(%true) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 3 : i64} : (i1, i1) -> () loc(#loc48)
    %555 = arith.ori %534, %ready_29 : i1
    %556 = arith.andi %value_28, %555 : i1 loc(#loc43)
    acir.path %value_28 {branch = 12 : i64, kind = "branch", txn = 4 : i64} loc(#loc43)
    acir.path %value_28 {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc43)
    %557 = arith.ori %534, %555 : i1 loc(#loc43)
    %558 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    %value_34, %ready_35 = acir.read(%value_28) {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc49)
    %559 = arith.andi %557, %ready_35 : i1 loc(#loc49)
    %560 = arith.ori %534, %559 : i1 loc(#loc49)
    %561 = arith.andi %556, %560 : i1 loc(#loc49)
    %562 = arith.cmpi ne, %value_34, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc49)
    %563 = arith.andi %561, %560 : i1
    %564 = arith.andi %value_28, %562 : i1
    %565 = arith.cmpi eq, %value_34, %c0_i32 : i32 loc(#loc49)
    %566 = arith.andi %value_28, %565 : i1
    %567 = arith.xori %566, %true : i1 loc(#loc50)
    %568 = arith.ori %567, %559 : i1
    %569 = arith.xori %564, %true : i1 loc(#loc51)
    %570 = arith.ori %569, %559 : i1
    acir.path %564 {branch = 13 : i64, kind = "branch", txn = 4 : i64} loc(#loc51)
    %value_36, %ready_37 = acir.read(%564) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc51)
    %571 = arith.andi %570, %ready_37 : i1 loc(#loc51)
    %572 = arith.ori %569, %571 : i1 loc(#loc51)
    %573 = arith.andi %563, %572 : i1 loc(#loc51)
    %574 = arith.subi %value_36, %c1_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc51)
    %575 = arith.andi %573, %572 : i1 loc(#loc52)
    %576 = arith.andi %566, %568 : i1
    %577 = arith.andi %564, %570 : i1
    %578 = arith.ori %576, %577 : i1
    %579 = arith.ori %534, %557 : i1
    %580 = arith.andi %579, %578 : i1
    %581 = arith.ori %534, %580 : i1
    %582 = arith.andi %575, %581 : i1
    %583 = arith.andi %value_28, %567 : i1
    %584 = arith.xori %583, %true : i1
    %585 = arith.ori %584, %580 : i1
    %586 = arith.ori %567, %580 : i1
    acir.path %566 {branch = 14 : i64, kind = "branch", txn = 4 : i64} loc(#loc50)
    %value_38, %ready_39 = acir.read(%566) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.request", txn = 4 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc50)
    %587 = arith.andi %586, %ready_39 : i1 loc(#loc50)
    %588 = arith.ori %567, %587 : i1 loc(#loc50)
    %589 = arith.andi %582, %588 : i1 loc(#loc50)
    %590 = arith.ori %567, %586 : i1 loc(#loc53)
    %591 = arith.andi %589, %590 : i1 loc(#loc53)
    %592 = acir.extract %value_38 {acir.id = "v26", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc54)
    %593 = acir.extract %value_38 {acir.id = "v27", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc55)
    %594 = acir.extract %value_38 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc56)
    %595 = acir.extract %594 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc56)
    %596 = acir.extract %value_38 {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc57)
    %597 = acir.extract %596 {acir.id = "v31", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc57)
    %598 = arith.muli %595, %597 {acir.id = "v32", acir.source_type = "u32"} : i32 loc(#loc56)
    %599 = acir.aggregate(%592, %593, %598) {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc58)
    %600 = arith.andi %583, %585 : i1
    %601 = arith.andi %566, %586 : i1
    %602 = arith.ori %600, %601 : i1
    %603 = arith.andi %566, %587 : i1
    %604 = arith.ori %600, %603 : i1
    %605 = arith.select %583, %558, %599 : !acir.struct<"src_payloads_25ebcaa7.Completion">
    %606 = arith.ori %534, %579 : i1
    %607 = arith.andi %604, %602 : i1
    %608 = arith.andi %607, %584 : i1
    %609 = arith.andi %606, %608 : i1
    %610 = arith.andi %value_28, %609 : i1
    %611 = arith.xori %610, %true : i1
    %612 = arith.ori %611, %555 : i1
    %613 = arith.andi %591, %612 : i1
    acir.path %610 {branch = 15 : i64, kind = "branch", txn = 4 : i64}
    %value_40, %ready_41 = acir.read(%610) {branch = 15 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mul_done", txn = 4 : i64} : (i1) -> (i1, i1)
    %614 = arith.andi %555, %ready_41 : i1
    %615 = arith.ori %611, %614 : i1
    %616 = arith.andi %613, %615 : i1
    %617 = arith.xori %value_40, %true : i1
    %618 = arith.andi %555, %617 : i1
    %619 = arith.ori %611, %618 : i1
    %620 = arith.andi %616, %619 : i1
    %621 = acir.fire "top.multiplier.advance#4" %620 {successors = [], txn = 4 : i64}
    %622 = arith.andi %621, %564 : i1
    acir.effect %622(%574) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1, i32) -> () loc(#loc52)
    %623 = arith.andi %621, %566 : i1
    acir.effect %623(%false) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 4 : i64} : (i1, i1) -> () loc(#loc53)
    %624 = arith.andi %621, %610 : i1
    acir.effect %624(%605) {branch = 15 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mul_done", txn = 4 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> ()
    %value_42, %ready_43 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.add_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc59)
    %625 = acir.unary %value_42 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc60)
    acir.path %true {branch = 16 : i64, kind = "branch", txn = 5 : i64} loc(#loc61)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc61)
    %626 = arith.xori %625, %true : i1 loc(#loc62)
    %627 = arith.ori %625, %ready_43 : i1
    acir.path %626 {branch = 17 : i64, kind = "branch", txn = 5 : i64} loc(#loc63)
    %value_44, %ready_45 = acir.read(%626) {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc63)
    %628 = arith.andi %627, %ready_45 : i1 loc(#loc63)
    %629 = arith.ori %625, %628 : i1 loc(#loc63)
    %630 = arith.andi %ready_43, %629 : i1 loc(#loc63)
    %631 = arith.andi %630, %629 : i1 loc(#loc63)
    %value_46, %ready_47 = acir.read(%626) {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "empty", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> (i1, i1) loc(#loc63)
    %632 = arith.andi %627, %ready_47 : i1 loc(#loc63)
    %633 = arith.ori %625, %632 : i1 loc(#loc63)
    %634 = arith.andi %631, %633 : i1 loc(#loc63)
    %635 = arith.xori %value_46, %true : i1 loc(#loc63)
    %636 = arith.andi %627, %635 : i1 loc(#loc63)
    %637 = arith.ori %625, %636 : i1 loc(#loc63)
    %638 = arith.andi %634, %637 : i1 loc(#loc63)
    %639 = arith.ori %626, %ready_43 : i1
    acir.path %625 {branch = 18 : i64, kind = "branch", txn = 5 : i64} loc(#loc62)
    %value_48, %ready_49 = acir.read(%625) {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc62)
    %640 = arith.andi %639, %ready_49 : i1 loc(#loc62)
    %641 = arith.ori %626, %640 : i1 loc(#loc62)
    %642 = arith.andi %638, %641 : i1 loc(#loc62)
    %643 = arith.andi %642, %641 : i1 loc(#loc62)
    %value_50, %ready_51 = acir.read(%625) {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "empty", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> (i1, i1) loc(#loc62)
    %644 = arith.andi %639, %ready_51 : i1 loc(#loc62)
    %645 = arith.ori %626, %644 : i1 loc(#loc62)
    %646 = arith.andi %643, %645 : i1 loc(#loc62)
    %647 = arith.xori %value_50, %true : i1 loc(#loc62)
    %648 = arith.andi %639, %647 : i1 loc(#loc62)
    %649 = arith.ori %626, %648 : i1 loc(#loc62)
    %650 = arith.andi %646, %649 : i1 loc(#loc62)
    %651 = arith.andi %626, %628 : i1
    %652 = arith.andi %625, %640 : i1
    %653 = arith.ori %651, %652 : i1
    %654 = arith.select %625, %value_48, %value_44 : !acir.struct<"src_payloads_25ebcaa7.Completion">
    %655 = acir.extract %654 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc64)
    %656 = acir.extract %654 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc65)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.rob:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc66)
    %657 = arith.subi %655, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc67)
    %658 = arith.andi %657, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc68)
    %659 = arith.cmpi eq, %658, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc69)
    %660 = arith.cmpi eq, %658, %c1_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc69)
    %661 = arith.cmpi eq, %658, %c2_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc69)
    %662 = arith.cmpi eq, %658, %c3_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc69)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc69)
    %663 = arith.andi %650, %653 : i1
    %664 = arith.cmpi ne, %658, %c0_i32 : i32 loc(#loc69)
    %665 = arith.cmpi eq, %658, %c0_i32 : i32 loc(#loc69)
    %666 = arith.ori %665, %653 : i1
    %667 = arith.ori %664, %653 : i1
    %668 = arith.andi %666, %653 : i1
    %669 = arith.andi %664, %668 : i1
    %670 = arith.andi %659, %667 : i1
    %671 = arith.ori %669, %670 : i1
    %672 = arith.select %664, %660, %true : i1
    %673 = arith.andi %663, %671 : i1
    %674 = arith.xori %672, %true : i1
    %675 = arith.ori %672, %671 : i1
    %676 = arith.ori %674, %671 : i1
    %677 = arith.andi %675, %653 : i1
    %678 = arith.andi %674, %677 : i1
    %679 = arith.andi %672, %676 : i1
    %680 = arith.ori %678, %679 : i1
    %681 = arith.select %672, %true, %661 : i1
    %682 = arith.andi %673, %680 : i1
    %683 = arith.xori %681, %true : i1
    %684 = arith.ori %681, %680 : i1
    %685 = arith.ori %683, %680 : i1
    %686 = arith.andi %684, %653 : i1
    %687 = arith.andi %683, %686 : i1
    %688 = arith.andi %681, %685 : i1
    %689 = arith.ori %687, %688 : i1
    %690 = arith.select %681, %true, %662 : i1
    %691 = arith.andi %689, %690 : i1 loc(#loc69)
    %692 = arith.andi %682, %691 : i1 loc(#loc69)
    %693 = arith.andi %691, %653 : i1 loc(#loc69)
    %694 = arith.andi %692, %693 : i1
    %695 = arith.ori %665, %693 : i1
    %696 = arith.andi %695, %693 : i1
    %697 = arith.ori %665, %696 : i1
    %698 = arith.andi %694, %697 : i1
    %699 = arith.andi %664, %660 : i1
    %700 = arith.cmpi ne, %658, %c1_i32 : i32 loc(#loc69)
    %701 = arith.andi %664, %700 : i1
    %702 = arith.xori %701, %true : i1
    %703 = arith.ori %702, %696 : i1
    %704 = arith.andi %703, %693 : i1
    %705 = arith.ori %702, %704 : i1
    %706 = arith.andi %698, %705 : i1
    %707 = arith.andi %701, %661 : i1
    %708 = arith.cmpi ne, %658, %c2_i32 : i32 loc(#loc69)
    %709 = arith.andi %701, %708 : i1
    %710 = arith.xori %709, %true : i1
    %711 = arith.ori %710, %704 : i1
    %712 = arith.andi %711, %693 : i1
    %713 = arith.ori %710, %712 : i1
    %714 = arith.andi %706, %713 : i1
    %715 = arith.andi %709, %662 : i1
    %716 = arith.xori %715, %true : i1 loc(#loc69)
    %717 = arith.ori %716, %712 : i1
    acir.path %715 {branch = 19 : i64, kind = "branch", txn = 5 : i64} loc(#loc69)
    acir.path %715 {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.3:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc69)
    %718 = arith.ori %716, %717 : i1 loc(#loc69)
    %719 = arith.andi %717, %693 : i1 loc(#loc69)
    %720 = arith.andi %715, %719 : i1 loc(#loc69)
    %value_52, %ready_53 = acir.read(%715) {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.3:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc70)
    %721 = arith.andi %718, %ready_53 : i1 loc(#loc70)
    %722 = arith.ori %716, %721 : i1 loc(#loc70)
    %723 = arith.andi %714, %722 : i1 loc(#loc70)
    %724 = acir.extract %value_52 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc71)
    %725 = arith.andi %723, %722 : i1
    %726 = arith.andi %715, %724 : i1
    %727 = arith.xori %724, %true : i1
    %728 = arith.andi %715, %727 : i1
    %729 = arith.xori %728, %true : i1
    %730 = arith.ori %729, %721 : i1
    %731 = arith.xori %726, %true : i1
    %732 = arith.ori %731, %721 : i1
    %733 = acir.extract %value_52 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc72)
    %734 = acir.unary %733 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc73)
    %735 = arith.andi %728, %730 : i1
    %736 = arith.andi %732, %ready_53 : i1
    %737 = arith.andi %726, %736 : i1
    %738 = arith.ori %735, %737 : i1
    %739 = arith.select %728, %false, %734 : i1
    %740 = arith.ori %716, %718 : i1
    %741 = arith.andi %740, %738 : i1
    %742 = arith.ori %716, %741 : i1
    %743 = arith.andi %725, %742 : i1
    %744 = arith.andi %715, %739 : i1
    %745 = arith.xori %739, %true : i1
    %746 = arith.andi %715, %745 : i1
    %747 = arith.xori %746, %true : i1
    %748 = arith.ori %747, %741 : i1
    %749 = arith.xori %744, %true : i1
    %750 = arith.ori %749, %741 : i1
    %751 = acir.extract %value_52 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc74)
    %752 = arith.andi %ready_53, %720 : i1 loc(#loc74)
    %753 = arith.cmpi eq, %751, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc74)
    %754 = arith.andi %746, %748 : i1
    %755 = arith.andi %750, %752 : i1
    %756 = arith.andi %744, %755 : i1
    %757 = arith.ori %754, %756 : i1
    %758 = arith.select %746, %false, %753 : i1
    %759 = arith.ori %716, %740 : i1
    %760 = arith.andi %759, %757 : i1
    %761 = arith.ori %716, %760 : i1
    %762 = arith.andi %743, %761 : i1
    %763 = arith.andi %715, %758 : i1
    %764 = arith.xori %763, %true : i1 loc(#loc75)
    %765 = arith.ori %764, %760 : i1
    %766 = acir.extract %value_52 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc76)
    %767 = acir.extract %value_52 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %768 = acir.extract %value_52 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %769 = acir.extract %value_52 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %770 = acir.aggregate(%766, %true, %767, %768, %769) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc76)
    %771 = acir.extract %770 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %772 = acir.extract %770 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %773 = acir.extract %770 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %774 = acir.extract %770 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %775 = acir.aggregate(%771, %772, %773, %774, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc77)
    %776 = arith.andi %765, %752 : i1 loc(#loc75)
    %777 = arith.ori %764, %776 : i1 loc(#loc75)
    %778 = arith.andi %762, %777 : i1 loc(#loc75)
    acir.path %763 {branch = 20 : i64, kind = "branch", txn = 5 : i64} loc(#loc75)
    %779 = arith.xori %707, %true : i1 loc(#loc69)
    %780 = arith.ori %779, %704 : i1
    acir.path %707 {branch = 21 : i64, kind = "branch", txn = 5 : i64} loc(#loc69)
    acir.path %707 {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.2:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc69)
    %781 = arith.ori %779, %780 : i1 loc(#loc69)
    %782 = arith.andi %780, %693 : i1 loc(#loc69)
    %783 = arith.andi %707, %782 : i1 loc(#loc69)
    %value_54, %ready_55 = acir.read(%707) {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.2:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc70)
    %784 = arith.andi %781, %ready_55 : i1 loc(#loc70)
    %785 = arith.ori %779, %784 : i1 loc(#loc70)
    %786 = arith.andi %778, %785 : i1 loc(#loc70)
    %787 = acir.extract %value_54 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc71)
    %788 = arith.andi %786, %785 : i1
    %789 = arith.andi %707, %787 : i1
    %790 = arith.xori %787, %true : i1
    %791 = arith.andi %707, %790 : i1
    %792 = arith.xori %791, %true : i1
    %793 = arith.ori %792, %784 : i1
    %794 = arith.xori %789, %true : i1
    %795 = arith.ori %794, %784 : i1
    %796 = acir.extract %value_54 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc72)
    %797 = acir.unary %796 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc73)
    %798 = arith.andi %791, %793 : i1
    %799 = arith.andi %795, %ready_55 : i1
    %800 = arith.andi %789, %799 : i1
    %801 = arith.ori %798, %800 : i1
    %802 = arith.select %791, %false, %797 : i1
    %803 = arith.ori %779, %781 : i1
    %804 = arith.andi %803, %801 : i1
    %805 = arith.ori %779, %804 : i1
    %806 = arith.andi %788, %805 : i1
    %807 = arith.andi %707, %802 : i1
    %808 = arith.xori %802, %true : i1
    %809 = arith.andi %707, %808 : i1
    %810 = arith.xori %809, %true : i1
    %811 = arith.ori %810, %804 : i1
    %812 = arith.xori %807, %true : i1
    %813 = arith.ori %812, %804 : i1
    %814 = acir.extract %value_54 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc74)
    %815 = arith.andi %ready_55, %783 : i1 loc(#loc74)
    %816 = arith.cmpi eq, %814, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc74)
    %817 = arith.andi %809, %811 : i1
    %818 = arith.andi %813, %815 : i1
    %819 = arith.andi %807, %818 : i1
    %820 = arith.ori %817, %819 : i1
    %821 = arith.select %809, %false, %816 : i1
    %822 = arith.ori %779, %803 : i1
    %823 = arith.andi %822, %820 : i1
    %824 = arith.ori %779, %823 : i1
    %825 = arith.andi %806, %824 : i1
    %826 = arith.andi %707, %821 : i1
    %827 = arith.xori %826, %true : i1 loc(#loc75)
    %828 = arith.ori %827, %823 : i1
    %829 = acir.extract %value_54 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc76)
    %830 = acir.extract %value_54 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %831 = acir.extract %value_54 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %832 = acir.extract %value_54 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %833 = acir.aggregate(%829, %true, %830, %831, %832) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc76)
    %834 = acir.extract %833 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %835 = acir.extract %833 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %836 = acir.extract %833 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %837 = acir.extract %833 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %838 = acir.aggregate(%834, %835, %836, %837, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc77)
    %839 = arith.andi %828, %815 : i1 loc(#loc75)
    %840 = arith.ori %827, %839 : i1 loc(#loc75)
    %841 = arith.andi %825, %840 : i1 loc(#loc75)
    acir.path %826 {branch = 22 : i64, kind = "branch", txn = 5 : i64} loc(#loc75)
    %842 = arith.xori %699, %true : i1 loc(#loc69)
    %843 = arith.ori %842, %696 : i1
    acir.path %699 {branch = 23 : i64, kind = "branch", txn = 5 : i64} loc(#loc69)
    acir.path %699 {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.1:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc69)
    %844 = arith.ori %842, %843 : i1 loc(#loc69)
    %845 = arith.andi %843, %693 : i1 loc(#loc69)
    %846 = arith.andi %699, %845 : i1 loc(#loc69)
    %value_56, %ready_57 = acir.read(%699) {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.1:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc70)
    %847 = arith.andi %844, %ready_57 : i1 loc(#loc70)
    %848 = arith.ori %842, %847 : i1 loc(#loc70)
    %849 = arith.andi %841, %848 : i1 loc(#loc70)
    %850 = acir.extract %value_56 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc71)
    %851 = arith.andi %849, %848 : i1
    %852 = arith.andi %699, %850 : i1
    %853 = arith.xori %850, %true : i1
    %854 = arith.andi %699, %853 : i1
    %855 = arith.xori %854, %true : i1
    %856 = arith.ori %855, %847 : i1
    %857 = arith.xori %852, %true : i1
    %858 = arith.ori %857, %847 : i1
    %859 = acir.extract %value_56 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc72)
    %860 = acir.unary %859 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc73)
    %861 = arith.andi %854, %856 : i1
    %862 = arith.andi %858, %ready_57 : i1
    %863 = arith.andi %852, %862 : i1
    %864 = arith.ori %861, %863 : i1
    %865 = arith.select %854, %false, %860 : i1
    %866 = arith.ori %842, %844 : i1
    %867 = arith.andi %866, %864 : i1
    %868 = arith.ori %842, %867 : i1
    %869 = arith.andi %851, %868 : i1
    %870 = arith.andi %699, %865 : i1
    %871 = arith.xori %865, %true : i1
    %872 = arith.andi %699, %871 : i1
    %873 = arith.xori %872, %true : i1
    %874 = arith.ori %873, %867 : i1
    %875 = arith.xori %870, %true : i1
    %876 = arith.ori %875, %867 : i1
    %877 = acir.extract %value_56 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc74)
    %878 = arith.andi %ready_57, %846 : i1 loc(#loc74)
    %879 = arith.cmpi eq, %877, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc74)
    %880 = arith.andi %872, %874 : i1
    %881 = arith.andi %876, %878 : i1
    %882 = arith.andi %870, %881 : i1
    %883 = arith.ori %880, %882 : i1
    %884 = arith.select %872, %false, %879 : i1
    %885 = arith.ori %842, %866 : i1
    %886 = arith.andi %885, %883 : i1
    %887 = arith.ori %842, %886 : i1
    %888 = arith.andi %869, %887 : i1
    %889 = arith.andi %699, %884 : i1
    %890 = arith.xori %889, %true : i1 loc(#loc75)
    %891 = arith.ori %890, %886 : i1
    %892 = acir.extract %value_56 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc76)
    %893 = acir.extract %value_56 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %894 = acir.extract %value_56 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %895 = acir.extract %value_56 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %896 = acir.aggregate(%892, %true, %893, %894, %895) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc76)
    %897 = acir.extract %896 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %898 = acir.extract %896 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %899 = acir.extract %896 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %900 = acir.extract %896 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %901 = acir.aggregate(%897, %898, %899, %900, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc77)
    %902 = arith.andi %891, %878 : i1 loc(#loc75)
    %903 = arith.ori %890, %902 : i1 loc(#loc75)
    %904 = arith.andi %888, %903 : i1 loc(#loc75)
    acir.path %889 {branch = 24 : i64, kind = "branch", txn = 5 : i64} loc(#loc75)
    %905 = arith.ori %664, %693 : i1
    acir.path %659 {branch = 25 : i64, kind = "branch", txn = 5 : i64} loc(#loc69)
    acir.path %659 {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.0:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc69)
    %906 = arith.ori %664, %905 : i1 loc(#loc69)
    %907 = arith.andi %905, %693 : i1 loc(#loc69)
    %908 = arith.andi %659, %907 : i1 loc(#loc69)
    %value_58, %ready_59 = acir.read(%659) {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.0:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc70)
    %909 = arith.andi %906, %ready_59 : i1 loc(#loc70)
    %910 = arith.ori %664, %909 : i1 loc(#loc70)
    %911 = arith.andi %904, %910 : i1 loc(#loc70)
    %912 = acir.extract %value_58 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc71)
    %913 = arith.andi %911, %910 : i1
    %914 = arith.andi %659, %912 : i1
    %915 = arith.xori %912, %true : i1
    %916 = arith.andi %659, %915 : i1
    %917 = arith.xori %916, %true : i1
    %918 = arith.ori %917, %909 : i1
    %919 = arith.xori %914, %true : i1
    %920 = arith.ori %919, %909 : i1
    %921 = acir.extract %value_58 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc72)
    %922 = acir.unary %921 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc73)
    %923 = arith.andi %916, %918 : i1
    %924 = arith.andi %920, %ready_59 : i1
    %925 = arith.andi %914, %924 : i1
    %926 = arith.ori %923, %925 : i1
    %927 = arith.select %916, %false, %922 : i1
    %928 = arith.ori %664, %906 : i1
    %929 = arith.andi %928, %926 : i1
    %930 = arith.ori %664, %929 : i1
    %931 = arith.andi %913, %930 : i1
    %932 = arith.andi %659, %927 : i1
    %933 = arith.xori %927, %true : i1
    %934 = arith.andi %659, %933 : i1
    %935 = arith.xori %934, %true : i1
    %936 = arith.ori %935, %929 : i1
    %937 = arith.xori %932, %true : i1
    %938 = arith.ori %937, %929 : i1
    %939 = acir.extract %value_58 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc74)
    %940 = arith.andi %ready_59, %908 : i1 loc(#loc74)
    %941 = arith.cmpi eq, %939, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc74)
    %942 = arith.andi %934, %936 : i1
    %943 = arith.andi %938, %940 : i1
    %944 = arith.andi %932, %943 : i1
    %945 = arith.ori %942, %944 : i1
    %946 = arith.select %934, %false, %941 : i1
    %947 = arith.ori %664, %928 : i1
    %948 = arith.andi %947, %945 : i1
    %949 = arith.ori %664, %948 : i1
    %950 = arith.andi %931, %949 : i1
    %951 = arith.andi %659, %946 : i1
    %952 = arith.xori %951, %true : i1 loc(#loc75)
    %953 = arith.ori %952, %948 : i1
    %954 = acir.extract %value_58 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc76)
    %955 = acir.extract %value_58 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %956 = acir.extract %value_58 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %957 = acir.extract %value_58 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %958 = acir.aggregate(%954, %true, %955, %956, %957) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc76)
    %959 = acir.extract %958 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %960 = acir.extract %958 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc77)
    %961 = acir.extract %958 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %962 = acir.extract %958 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc77)
    %963 = acir.aggregate(%959, %960, %961, %962, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc77)
    %964 = arith.andi %953, %940 : i1 loc(#loc75)
    %965 = arith.ori %952, %964 : i1 loc(#loc75)
    %966 = arith.andi %950, %965 : i1 loc(#loc75)
    acir.path %951 {branch = 26 : i64, kind = "branch", txn = 5 : i64} loc(#loc75)
    %967 = acir.extract %654 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc78)
    %968 = acir.extract %654 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc79)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc80)
    %value_60, %ready_61 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc81)
    %969 = arith.andi %966, %ready_61 : i1 loc(#loc81)
    %970 = acir.extract %value_60 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc82)
    %971 = arith.xori %970, %true : i1
    %972 = arith.ori %970, %ready_61 : i1
    %973 = arith.ori %971, %ready_61 : i1
    %974 = acir.extract %value_60 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc83)
    %975 = acir.extract %974 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc83)
    %976 = acir.unary %975 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc84)
    %977 = arith.andi %973, %ready_61 : i1
    %978 = arith.ori %971, %977 : i1
    %979 = arith.andi %969, %978 : i1
    %980 = arith.andi %970, %976 : i1
    %981 = arith.xori %976, %true : i1
    %982 = arith.andi %970, %981 : i1
    %983 = arith.xori %982, %true : i1
    %984 = arith.ori %983, %977 : i1
    %985 = arith.xori %980, %true : i1
    %986 = arith.ori %985, %977 : i1
    %987 = acir.extract %value_60 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc85)
    %988 = acir.extract %987 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc85)
    %989 = arith.andi %ready_61, %653 : i1 loc(#loc85)
    %990 = arith.cmpi eq, %988, %967 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc85)
    %991 = arith.andi %982, %984 : i1
    %992 = arith.andi %986, %989 : i1
    %993 = arith.andi %980, %992 : i1
    %994 = arith.ori %991, %993 : i1
    %995 = arith.select %982, %false, %990 : i1
    %996 = arith.ori %971, %973 : i1
    %997 = arith.andi %996, %994 : i1
    %998 = arith.ori %971, %997 : i1
    %999 = arith.andi %979, %998 : i1
    %1000 = arith.andi %970, %995 : i1
    %1001 = arith.xori %995, %true : i1
    %1002 = arith.andi %970, %1001 : i1
    %1003 = arith.xori %1002, %true : i1
    %1004 = arith.ori %1003, %997 : i1
    %1005 = arith.xori %1000, %true : i1
    %1006 = arith.ori %1005, %997 : i1
    %1007 = acir.aggregate(%true, %967, %968) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc86)
    %1008 = acir.extract %value_60 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc87)
    %1009 = acir.extract %value_60 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc87)
    %1010 = acir.extract %value_60 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc87)
    %1011 = acir.extract %value_60 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc87)
    %1012 = acir.extract %value_60 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc87)
    %1013 = acir.aggregate(%1008, %1009, %1010, %1011, %1007, %1012) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc87)
    %1014 = arith.andi %1002, %1004 : i1
    %1015 = arith.andi %1000, %1006 : i1
    %1016 = arith.ori %1014, %1015 : i1
    %1017 = arith.andi %1004, %ready_61 : i1
    %1018 = arith.andi %1002, %1017 : i1
    %1019 = arith.andi %1006, %989 : i1
    %1020 = arith.andi %1000, %1019 : i1
    %1021 = arith.ori %1018, %1020 : i1
    %1022 = arith.select %1002, %value_60, %1013 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1023 = arith.ori %971, %996 : i1
    %1024 = acir.extract %1022 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc88)
    %1025 = acir.extract %1024 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc88)
    %1026 = acir.unary %1025 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc89)
    %1027 = arith.andi %1023, %1021 : i1
    %1028 = arith.ori %971, %1027 : i1
    %1029 = arith.andi %999, %1028 : i1
    %1030 = arith.andi %970, %1026 : i1
    %1031 = arith.xori %1026, %true : i1
    %1032 = arith.andi %970, %1031 : i1
    %1033 = arith.xori %1032, %true : i1
    %1034 = arith.ori %1033, %1027 : i1
    %1035 = arith.xori %1030, %true : i1
    %1036 = arith.ori %1035, %1027 : i1
    %1037 = acir.extract %1022 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc90)
    %1038 = acir.extract %1037 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc90)
    %1039 = arith.andi %1021, %653 : i1 loc(#loc90)
    %1040 = arith.cmpi eq, %1038, %967 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc90)
    %1041 = arith.andi %1032, %1034 : i1
    %1042 = arith.andi %1036, %1039 : i1
    %1043 = arith.andi %1030, %1042 : i1
    %1044 = arith.ori %1041, %1043 : i1
    %1045 = arith.select %1032, %false, %1040 : i1
    %1046 = arith.ori %971, %1023 : i1
    %1047 = arith.andi %1046, %1044 : i1
    %1048 = arith.ori %971, %1047 : i1
    %1049 = arith.andi %1029, %1048 : i1
    %1050 = arith.andi %970, %1045 : i1
    %1051 = arith.xori %1045, %true : i1
    %1052 = arith.andi %970, %1051 : i1
    %1053 = arith.xori %1052, %true : i1
    %1054 = arith.ori %1053, %1047 : i1
    %1055 = arith.xori %1050, %true : i1
    %1056 = arith.ori %1055, %1047 : i1
    %1057 = acir.aggregate(%true, %967, %968) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc91)
    %1058 = acir.extract %1022 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc92)
    %1059 = acir.extract %1022 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1060 = acir.extract %1022 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1061 = acir.extract %1022 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1062 = acir.extract %1022 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc92)
    %1063 = acir.aggregate(%1058, %1059, %1060, %1061, %1062, %1057) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %1064 = arith.andi %1054, %1016 : i1
    %1065 = arith.andi %1052, %1064 : i1
    %1066 = arith.andi %1050, %1056 : i1
    %1067 = arith.ori %1065, %1066 : i1
    %1068 = arith.select %1052, %1003, %true : i1
    %1069 = arith.andi %1054, %1021 : i1
    %1070 = arith.andi %1052, %1069 : i1
    %1071 = arith.andi %1056, %1039 : i1
    %1072 = arith.andi %1050, %1071 : i1
    %1073 = arith.ori %1070, %1072 : i1
    %1074 = arith.select %1052, %1022, %1063 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1075 = arith.ori %971, %1046 : i1
    %1076 = arith.andi %971, %972 : i1
    %1077 = arith.andi %1075, %1067 : i1
    %1078 = arith.andi %970, %1077 : i1
    %1079 = arith.ori %1076, %1078 : i1
    %1080 = arith.select %970, %1068, %false : i1
    %1081 = arith.andi %972, %ready_61 : i1
    %1082 = arith.andi %971, %1081 : i1
    %1083 = arith.andi %1075, %1073 : i1
    %1084 = arith.andi %970, %1083 : i1
    %1085 = arith.ori %1082, %1084 : i1
    %1086 = arith.select %970, %1074, %value_60 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1087 = arith.andi %1049, %1079 : i1
    %1088 = arith.xori %1080, %true : i1 loc(#loc93)
    %1089 = arith.ori %1088, %1079 : i1
    %1090 = arith.andi %1089, %1085 : i1 loc(#loc93)
    %1091 = arith.ori %1088, %1090 : i1 loc(#loc93)
    %1092 = arith.andi %1087, %1091 : i1 loc(#loc93)
    acir.path %1080 {branch = 27 : i64, kind = "branch", txn = 5 : i64} loc(#loc93)
    %1093 = acir.extract %654 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc94)
    %1094 = acir.extract %654 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc95)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc96)
    %value_62, %ready_63 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc81)
    %1095 = arith.andi %1092, %ready_63 : i1 loc(#loc81)
    %1096 = acir.extract %value_62 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc82)
    %1097 = arith.xori %1096, %true : i1
    %1098 = arith.ori %1096, %ready_63 : i1
    %1099 = arith.ori %1097, %ready_63 : i1
    %1100 = acir.extract %value_62 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc83)
    %1101 = acir.extract %1100 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc83)
    %1102 = acir.unary %1101 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc84)
    %1103 = arith.andi %1099, %ready_63 : i1
    %1104 = arith.ori %1097, %1103 : i1
    %1105 = arith.andi %1095, %1104 : i1
    %1106 = arith.andi %1096, %1102 : i1
    %1107 = arith.xori %1102, %true : i1
    %1108 = arith.andi %1096, %1107 : i1
    %1109 = arith.xori %1108, %true : i1
    %1110 = arith.ori %1109, %1103 : i1
    %1111 = arith.xori %1106, %true : i1
    %1112 = arith.ori %1111, %1103 : i1
    %1113 = acir.extract %value_62 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc85)
    %1114 = acir.extract %1113 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc85)
    %1115 = arith.andi %ready_63, %653 : i1 loc(#loc85)
    %1116 = arith.cmpi eq, %1114, %1093 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc85)
    %1117 = arith.andi %1108, %1110 : i1
    %1118 = arith.andi %1112, %1115 : i1
    %1119 = arith.andi %1106, %1118 : i1
    %1120 = arith.ori %1117, %1119 : i1
    %1121 = arith.select %1108, %false, %1116 : i1
    %1122 = arith.ori %1097, %1099 : i1
    %1123 = arith.andi %1122, %1120 : i1
    %1124 = arith.ori %1097, %1123 : i1
    %1125 = arith.andi %1105, %1124 : i1
    %1126 = arith.andi %1096, %1121 : i1
    %1127 = arith.xori %1121, %true : i1
    %1128 = arith.andi %1096, %1127 : i1
    %1129 = arith.xori %1128, %true : i1
    %1130 = arith.ori %1129, %1123 : i1
    %1131 = arith.xori %1126, %true : i1
    %1132 = arith.ori %1131, %1123 : i1
    %1133 = acir.aggregate(%true, %1093, %1094) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc86)
    %1134 = acir.extract %value_62 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc87)
    %1135 = acir.extract %value_62 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc87)
    %1136 = acir.extract %value_62 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc87)
    %1137 = acir.extract %value_62 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc87)
    %1138 = acir.extract %value_62 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc87)
    %1139 = acir.aggregate(%1134, %1135, %1136, %1137, %1133, %1138) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc87)
    %1140 = arith.andi %1128, %1130 : i1
    %1141 = arith.andi %1126, %1132 : i1
    %1142 = arith.ori %1140, %1141 : i1
    %1143 = arith.andi %1130, %ready_63 : i1
    %1144 = arith.andi %1128, %1143 : i1
    %1145 = arith.andi %1132, %1115 : i1
    %1146 = arith.andi %1126, %1145 : i1
    %1147 = arith.ori %1144, %1146 : i1
    %1148 = arith.select %1128, %value_62, %1139 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1149 = arith.ori %1097, %1122 : i1
    %1150 = acir.extract %1148 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc88)
    %1151 = acir.extract %1150 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc88)
    %1152 = acir.unary %1151 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc89)
    %1153 = arith.andi %1149, %1147 : i1
    %1154 = arith.ori %1097, %1153 : i1
    %1155 = arith.andi %1125, %1154 : i1
    %1156 = arith.andi %1096, %1152 : i1
    %1157 = arith.xori %1152, %true : i1
    %1158 = arith.andi %1096, %1157 : i1
    %1159 = arith.xori %1158, %true : i1
    %1160 = arith.ori %1159, %1153 : i1
    %1161 = arith.xori %1156, %true : i1
    %1162 = arith.ori %1161, %1153 : i1
    %1163 = acir.extract %1148 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc90)
    %1164 = acir.extract %1163 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc90)
    %1165 = arith.andi %1147, %653 : i1 loc(#loc90)
    %1166 = arith.cmpi eq, %1164, %1093 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc90)
    %1167 = arith.andi %1158, %1160 : i1
    %1168 = arith.andi %1162, %1165 : i1
    %1169 = arith.andi %1156, %1168 : i1
    %1170 = arith.ori %1167, %1169 : i1
    %1171 = arith.select %1158, %false, %1166 : i1
    %1172 = arith.ori %1097, %1149 : i1
    %1173 = arith.andi %1172, %1170 : i1
    %1174 = arith.ori %1097, %1173 : i1
    %1175 = arith.andi %1155, %1174 : i1
    %1176 = arith.andi %1096, %1171 : i1
    %1177 = arith.xori %1171, %true : i1
    %1178 = arith.andi %1096, %1177 : i1
    %1179 = arith.xori %1178, %true : i1
    %1180 = arith.ori %1179, %1173 : i1
    %1181 = arith.xori %1176, %true : i1
    %1182 = arith.ori %1181, %1173 : i1
    %1183 = acir.aggregate(%true, %1093, %1094) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc91)
    %1184 = acir.extract %1148 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc92)
    %1185 = acir.extract %1148 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1186 = acir.extract %1148 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1187 = acir.extract %1148 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1188 = acir.extract %1148 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc92)
    %1189 = acir.aggregate(%1184, %1185, %1186, %1187, %1188, %1183) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %1190 = arith.andi %1180, %1142 : i1
    %1191 = arith.andi %1178, %1190 : i1
    %1192 = arith.andi %1176, %1182 : i1
    %1193 = arith.ori %1191, %1192 : i1
    %1194 = arith.select %1178, %1129, %true : i1
    %1195 = arith.andi %1180, %1147 : i1
    %1196 = arith.andi %1178, %1195 : i1
    %1197 = arith.andi %1182, %1165 : i1
    %1198 = arith.andi %1176, %1197 : i1
    %1199 = arith.ori %1196, %1198 : i1
    %1200 = arith.select %1178, %1148, %1189 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1201 = arith.ori %1097, %1172 : i1
    %1202 = arith.andi %1097, %1098 : i1
    %1203 = arith.andi %1201, %1193 : i1
    %1204 = arith.andi %1096, %1203 : i1
    %1205 = arith.ori %1202, %1204 : i1
    %1206 = arith.select %1096, %1194, %false : i1
    %1207 = arith.andi %1098, %ready_63 : i1
    %1208 = arith.andi %1097, %1207 : i1
    %1209 = arith.andi %1201, %1199 : i1
    %1210 = arith.andi %1096, %1209 : i1
    %1211 = arith.ori %1208, %1210 : i1
    %1212 = arith.select %1096, %1200, %value_62 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1213 = arith.andi %1175, %1205 : i1
    %1214 = arith.xori %1206, %true : i1 loc(#loc93)
    %1215 = arith.ori %1214, %1205 : i1
    %1216 = arith.andi %1215, %1211 : i1 loc(#loc93)
    %1217 = arith.ori %1214, %1216 : i1 loc(#loc93)
    %1218 = arith.andi %1213, %1217 : i1 loc(#loc93)
    acir.path %1206 {branch = 28 : i64, kind = "branch", txn = 5 : i64} loc(#loc93)
    acir.path %653 {branch = 29 : i64, kind = "branch", txn = 5 : i64}
    %1219 = acir.fire "top.cdb.broadcast#5" %1218 {successors = [2, 4, 8], txn = 5 : i64}
    %1220 = arith.andi %1219, %626 : i1
    acir.effect %1220() {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> () loc(#loc63)
    %1221 = arith.andi %1219, %625 : i1
    acir.effect %1221() {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> () loc(#loc62)
    %1222 = arith.andi %1219, %763 : i1
    acir.effect %1222(%775) {branch = 20 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.3:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc75)
    %1223 = arith.andi %1219, %826 : i1
    acir.effect %1223(%838) {branch = 22 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.2:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc75)
    %1224 = arith.andi %1219, %889 : i1
    acir.effect %1224(%901) {branch = 24 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.1:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc75)
    %1225 = arith.andi %1219, %951 : i1
    acir.effect %1225(%963) {branch = 26 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.0:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc75)
    %1226 = arith.andi %1219, %1080 : i1
    acir.effect %1226(%1086) {branch = 27 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc93)
    %1227 = arith.andi %1219, %1206 : i1
    acir.effect %1227(%1212) {branch = 28 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc93)
    %1228 = arith.andi %1219, %653 : i1
    acir.effect %1228(%654) {branch = 29 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.event", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> ()
    %value_64, %ready_65 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.add_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc97)
    %1229 = arith.xori %value_64, %true : i1
    %1230 = arith.ori %value_64, %ready_65 : i1
    %1231 = arith.ori %1229, %ready_65 : i1
    acir.path %value_64 {branch = 30 : i64, kind = "branch", txn = -1 : i64} loc(#loc98)
    %value_66, %ready_67 = acir.read(%value_64) {branch = 30 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.mul_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc98)
    %1232 = arith.andi %1229, %1230 : i1
    %1233 = arith.andi %1231, %ready_67 : i1
    %1234 = arith.andi %value_64, %1233 : i1
    %1235 = arith.ori %1232, %1234 : i1
    %1236 = arith.select %value_64, %value_66, %false : i1
    %1237 = arith.xori %1236, %true : i1 loc(#loc99)
    %1238 = arith.ori %1237, %1235 : i1
    %1239 = arith.andi %1236, %1238 : i1 loc(#loc99)
    acir.path %1236 {branch = 31 : i64, kind = "branch", txn = 6 : i64} loc(#loc99)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc99)
    %1240 = arith.ori %1237, %1238 : i1 loc(#loc99)
    %value_68, %ready_69 = acir.read(%1236) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "peek", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, i1) loc(#loc100)
    %1241 = arith.andi %1240, %ready_69 : i1 loc(#loc100)
    %1242 = arith.ori %1237, %1241 : i1 loc(#loc100)
    %1243 = arith.andi %1239, %1242 : i1 loc(#loc100)
    %1244 = arith.andi %1243, %1242 : i1 loc(#loc100)
    %value_70, %ready_71 = acir.read(%1236) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "empty", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (i1, i1) loc(#loc100)
    %1245 = arith.andi %1240, %ready_71 : i1 loc(#loc100)
    %1246 = arith.ori %1237, %1245 : i1 loc(#loc100)
    %1247 = arith.andi %1244, %1246 : i1 loc(#loc100)
    %1248 = arith.xori %value_70, %true : i1 loc(#loc100)
    %1249 = arith.andi %1240, %1248 : i1 loc(#loc100)
    %1250 = arith.ori %1237, %1249 : i1 loc(#loc100)
    %1251 = arith.andi %1247, %1250 : i1 loc(#loc100)
    %1252 = acir.extract %value_68 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc101)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc102)
    %1253 = arith.ori %1237, %1240 : i1 loc(#loc102)
    %1254 = arith.andi %1236, %1241 : i1 loc(#loc102)
    %1255 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1256 = arith.cmpi ult, %1252, %c8_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc103)
    %1257 = arith.andi %1253, %1254 : i1 loc(#loc104)
    %1258 = arith.ori %1237, %1257 : i1 loc(#loc104)
    %1259 = arith.andi %1251, %1258 : i1 loc(#loc104)
    %1260 = arith.cmpi eq, %1252, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc105)
    %1261 = arith.andi %1259, %1258 : i1
    %1262 = arith.andi %1236, %1260 : i1
    %1263 = arith.cmpi ne, %1252, %c0_i32 : i32 loc(#loc105)
    %1264 = arith.andi %1236, %1263 : i1
    %1265 = arith.xori %1264, %true : i1 loc(#loc106)
    %1266 = arith.ori %1265, %1257 : i1
    %1267 = arith.xori %1262, %true : i1
    %1268 = arith.ori %1267, %1257 : i1
    %1269 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc107)
    %1270 = arith.andi %1264, %1266 : i1
    %1271 = arith.andi %1262, %1268 : i1
    %1272 = arith.ori %1270, %1271 : i1
    %1273 = arith.select %1264, %1255, %1269 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1274 = arith.ori %1237, %1253 : i1 loc(#loc108)
    %1275 = arith.andi %1274, %1272 : i1
    %1276 = arith.ori %1237, %1275 : i1
    %1277 = arith.andi %1261, %1276 : i1
    %1278 = arith.andi %1236, %1265 : i1
    %1279 = arith.xori %1278, %true : i1
    %1280 = arith.ori %1279, %1275 : i1
    %1281 = arith.ori %1265, %1275 : i1
    acir.path %1264 {branch = 32 : i64, kind = "branch", txn = 6 : i64} loc(#loc106)
    %value_72, %ready_73 = acir.read(%1264) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc106)
    %1282 = arith.andi %1281, %ready_73 : i1 loc(#loc106)
    %1283 = arith.ori %1265, %1282 : i1 loc(#loc106)
    %1284 = arith.andi %1277, %1283 : i1 loc(#loc106)
    %1285 = arith.andi %ready_73, %1254 : i1 loc(#loc106)
    %1286 = arith.extui %1252 : i32 to i64 loc(#loc106)
    %1287 = arith.cmpi ult, %1286, %c8_i64 : i64 loc(#loc106)
    %1288 = arith.select %1287, %1286, %c0_i64 : i64 loc(#loc106)
    %1289 = arith.andi %1285, %1287 : i1 loc(#loc106)
    %1290 = "acir.array.index"(%value_72, %1288) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc106)
    %value_74, %ready_75 = acir.read(%1264) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc109)
    %1291 = arith.andi %1281, %ready_75 : i1 loc(#loc109)
    %1292 = arith.ori %1265, %1291 : i1 loc(#loc109)
    %1293 = arith.andi %1284, %1292 : i1 loc(#loc109)
    %1294 = arith.andi %ready_75, %1254 : i1 loc(#loc109)
    %1295 = arith.andi %1294, %1287 : i1 loc(#loc109)
    %1296 = "acir.array.index"(%value_74, %1288) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc109)
    %1297 = arith.andi %1289, %1295 : i1 loc(#loc110)
    %1298 = arith.cmpi eq, %1290, %1296 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc110)
    %1299 = arith.andi %1281, %1297 : i1
    %1300 = arith.ori %1265, %1299 : i1
    %1301 = arith.andi %1293, %1300 : i1
    %1302 = arith.andi %1264, %1298 : i1
    %1303 = arith.cmpi ne, %1290, %1296 : i32 loc(#loc110)
    %1304 = arith.andi %1264, %1303 : i1
    %1305 = arith.xori %1304, %true : i1
    %1306 = arith.ori %1305, %1299 : i1
    %1307 = arith.xori %1302, %true : i1 loc(#loc111)
    %1308 = arith.ori %1307, %1299 : i1
    acir.path %1302 {branch = 33 : i64, kind = "branch", txn = 6 : i64} loc(#loc111)
    %value_76, %ready_77 = acir.read(%1302) {branch = 33 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc111)
    %1309 = arith.andi %1308, %ready_77 : i1 loc(#loc111)
    %1310 = arith.ori %1307, %1309 : i1 loc(#loc111)
    %1311 = arith.andi %1301, %1310 : i1 loc(#loc111)
    %1312 = arith.andi %ready_77, %1254 : i1 loc(#loc111)
    %1313 = arith.andi %1312, %1287 : i1 loc(#loc111)
    %1314 = "acir.array.index"(%value_76, %1288) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc111)
    %1315 = arith.andi %1289, %1313 : i1 loc(#loc112)
    %1316 = acir.aggregate(%true, %1290, %1314) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc112)
    %1317 = arith.andi %1306, %1272 : i1
    %1318 = arith.andi %1304, %1317 : i1
    %1319 = arith.andi %1302, %1308 : i1
    %1320 = arith.ori %1318, %1319 : i1
    %1321 = arith.select %1304, %1264, %false : i1
    %1322 = arith.andi %1308, %1315 : i1
    %1323 = arith.andi %1302, %1322 : i1
    %1324 = arith.ori %1318, %1323 : i1
    %1325 = arith.select %1304, %1273, %1316 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1326 = arith.ori %1265, %1281 : i1
    %1327 = arith.andi %1326, %1320 : i1
    %1328 = arith.ori %1265, %1327 : i1
    %1329 = arith.andi %1311, %1328 : i1
    %1330 = arith.andi %1264, %1321 : i1
    %1331 = arith.xori %1321, %true : i1
    %1332 = arith.andi %1264, %1331 : i1
    %1333 = arith.xori %1332, %true : i1
    %1334 = arith.ori %1333, %1327 : i1
    %1335 = arith.xori %1330, %true : i1 loc(#loc113)
    %1336 = arith.ori %1335, %1327 : i1
    acir.path %1330 {branch = 34 : i64, kind = "branch", txn = 6 : i64} loc(#loc113)
    acir.path %1330 {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc113)
    %1337 = arith.ori %1335, %1336 : i1 loc(#loc113)
    %1338 = arith.andi %1336, %1289 : i1 loc(#loc113)
    %1339 = arith.andi %1330, %1338 : i1 loc(#loc113)
    %1340 = arith.subi %1290, %c1_i32 {acir.id = "v20", acir.source_type = "u32"} : i32 loc(#loc114)
    %1341 = arith.andi %1340, %c3_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc115)
    %1342 = arith.cmpi eq, %1341, %c0_i32 {acir.id = "v24", acir.source_type = "bool"} : i32 loc(#loc116)
    %1343 = arith.cmpi eq, %1341, %c1_i32 {acir.id = "v26", acir.source_type = "bool"} : i32 loc(#loc116)
    %1344 = arith.cmpi eq, %1341, %c2_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc116)
    %1345 = arith.cmpi eq, %1341, %c3_i32 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc116)
    acir.path %1330 {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1346 = arith.ori %1335, %1337 : i1 loc(#loc116)
    %1347 = arith.andi %1337, %1339 : i1 loc(#loc116)
    %1348 = arith.andi %1330, %1347 : i1 loc(#loc116)
    %1349 = arith.andi %1346, %1348 : i1
    %1350 = arith.ori %1335, %1349 : i1
    %1351 = arith.andi %1329, %1350 : i1
    %1352 = arith.andi %1330, %1342 : i1
    %1353 = arith.cmpi ne, %1341, %c0_i32 : i32 loc(#loc116)
    %1354 = arith.andi %1330, %1353 : i1
    %1355 = arith.xori %1354, %true : i1
    %1356 = arith.ori %1355, %1349 : i1
    %1357 = arith.xori %1352, %true : i1 loc(#loc116)
    %1358 = arith.ori %1357, %1349 : i1
    %1359 = arith.andi %1356, %1348 : i1
    %1360 = arith.andi %1354, %1359 : i1
    %1361 = arith.andi %1352, %1358 : i1
    %1362 = arith.ori %1360, %1361 : i1
    %1363 = arith.select %1354, %1343, %true : i1
    %1364 = arith.ori %1335, %1346 : i1
    %1365 = arith.andi %1364, %1362 : i1
    %1366 = arith.ori %1335, %1365 : i1
    %1367 = arith.andi %1351, %1366 : i1
    %1368 = arith.andi %1330, %1363 : i1
    %1369 = arith.xori %1363, %true : i1
    %1370 = arith.andi %1330, %1369 : i1
    %1371 = arith.xori %1370, %true : i1
    %1372 = arith.ori %1371, %1365 : i1
    %1373 = arith.xori %1368, %true : i1
    %1374 = arith.ori %1373, %1365 : i1
    %1375 = arith.andi %1372, %1348 : i1
    %1376 = arith.andi %1370, %1375 : i1
    %1377 = arith.andi %1368, %1374 : i1
    %1378 = arith.ori %1376, %1377 : i1
    %1379 = arith.select %1370, %1344, %true : i1
    %1380 = arith.ori %1335, %1364 : i1
    %1381 = arith.andi %1380, %1378 : i1
    %1382 = arith.ori %1335, %1381 : i1
    %1383 = arith.andi %1367, %1382 : i1
    %1384 = arith.andi %1330, %1379 : i1
    %1385 = arith.xori %1379, %true : i1
    %1386 = arith.andi %1330, %1385 : i1
    %1387 = arith.xori %1386, %true : i1
    %1388 = arith.ori %1387, %1381 : i1
    %1389 = arith.xori %1384, %true : i1
    %1390 = arith.ori %1389, %1381 : i1
    %1391 = arith.andi %1388, %1348 : i1
    %1392 = arith.andi %1386, %1391 : i1
    %1393 = arith.andi %1384, %1390 : i1
    %1394 = arith.ori %1392, %1393 : i1
    %1395 = arith.select %1386, %1345, %true : i1
    %1396 = arith.ori %1335, %1380 : i1
    %1397 = arith.andi %1396, %1394 : i1 loc(#loc116)
    %1398 = arith.andi %1330, %1397 : i1 loc(#loc116)
    %1399 = arith.andi %1398, %1395 : i1 loc(#loc116)
    %1400 = arith.andi %1337, %1399 : i1 loc(#loc116)
    %1401 = arith.ori %1335, %1400 : i1 loc(#loc116)
    %1402 = arith.andi %1383, %1401 : i1 loc(#loc116)
    %1403 = arith.andi %1400, %1339 : i1 loc(#loc116)
    %1404 = arith.andi %1330, %1403 : i1 loc(#loc116)
    %1405 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1406 = arith.andi %1401, %1404 : i1
    %1407 = arith.ori %1335, %1406 : i1
    %1408 = arith.andi %1402, %1407 : i1
    %1409 = arith.ori %1355, %1406 : i1
    %1410 = arith.andi %1409, %1404 : i1
    %1411 = arith.ori %1355, %1410 : i1
    %1412 = arith.andi %1408, %1411 : i1
    %1413 = arith.andi %1354, %1343 : i1
    %1414 = arith.cmpi ne, %1341, %c1_i32 : i32 loc(#loc116)
    %1415 = arith.andi %1354, %1414 : i1
    %1416 = arith.xori %1415, %true : i1
    %1417 = arith.ori %1416, %1410 : i1
    %1418 = arith.andi %1417, %1404 : i1
    %1419 = arith.ori %1416, %1418 : i1
    %1420 = arith.andi %1412, %1419 : i1
    %1421 = arith.andi %1415, %1344 : i1
    %1422 = arith.cmpi ne, %1341, %c2_i32 : i32 loc(#loc116)
    %1423 = arith.andi %1415, %1422 : i1
    %1424 = arith.xori %1423, %true : i1
    %1425 = arith.ori %1424, %1418 : i1
    %1426 = arith.andi %1425, %1404 : i1
    %1427 = arith.ori %1424, %1426 : i1
    %1428 = arith.andi %1420, %1427 : i1
    %1429 = arith.andi %1423, %1345 : i1
    %1430 = arith.cmpi ne, %1341, %c3_i32 : i32 loc(#loc116)
    %1431 = arith.andi %1423, %1430 : i1
    %1432 = arith.xori %1431, %true : i1
    %1433 = arith.ori %1432, %1426 : i1
    %1434 = arith.xori %1429, %true : i1 loc(#loc116)
    %1435 = arith.ori %1434, %1426 : i1
    acir.path %1429 {branch = 35 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1429 {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1436 = arith.ori %1434, %1435 : i1 loc(#loc116)
    %value_78, %ready_79 = acir.read(%1429) {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1437 = arith.andi %1436, %ready_79 : i1 loc(#loc117)
    %1438 = arith.ori %1434, %1437 : i1 loc(#loc117)
    %1439 = arith.andi %1428, %1438 : i1 loc(#loc117)
    %1440 = arith.ori %1434, %1436 : i1
    %1441 = arith.andi %1439, %1440 : i1
    %1442 = arith.andi %1429, %1437 : i1
    %1443 = arith.andi %1431, %1433 : i1
    %1444 = arith.andi %1429, %1435 : i1
    %1445 = arith.ori %1443, %1444 : i1
    %1446 = arith.andi %1435, %1442 : i1
    %1447 = arith.andi %1429, %1446 : i1
    %1448 = arith.ori %1443, %1447 : i1
    %1449 = arith.select %1431, %1405, %value_78 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1450 = arith.ori %1424, %1425 : i1
    %1451 = arith.xori %1421, %true : i1 loc(#loc116)
    %1452 = arith.ori %1451, %1418 : i1
    acir.path %1421 {branch = 36 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1421 {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1453 = arith.ori %1451, %1452 : i1 loc(#loc116)
    %value_80, %ready_81 = acir.read(%1421) {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1454 = arith.andi %1453, %ready_81 : i1 loc(#loc117)
    %1455 = arith.ori %1451, %1454 : i1 loc(#loc117)
    %1456 = arith.andi %1441, %1455 : i1 loc(#loc117)
    %1457 = arith.ori %1451, %1453 : i1
    %1458 = arith.andi %1456, %1457 : i1
    %1459 = arith.andi %1421, %1454 : i1
    %1460 = arith.andi %1450, %1445 : i1
    %1461 = arith.andi %1423, %1460 : i1
    %1462 = arith.andi %1421, %1452 : i1
    %1463 = arith.ori %1461, %1462 : i1
    %1464 = arith.select %1423, %1431, %false : i1
    %1465 = arith.andi %1450, %1448 : i1
    %1466 = arith.andi %1423, %1465 : i1
    %1467 = arith.andi %1452, %1459 : i1
    %1468 = arith.andi %1421, %1467 : i1
    %1469 = arith.ori %1466, %1468 : i1
    %1470 = arith.select %1423, %1449, %value_80 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1471 = arith.ori %1416, %1417 : i1
    %1472 = arith.xori %1413, %true : i1 loc(#loc116)
    %1473 = arith.ori %1472, %1410 : i1
    acir.path %1413 {branch = 37 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1413 {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1474 = arith.ori %1472, %1473 : i1 loc(#loc116)
    %value_82, %ready_83 = acir.read(%1413) {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1475 = arith.andi %1474, %ready_83 : i1 loc(#loc117)
    %1476 = arith.ori %1472, %1475 : i1 loc(#loc117)
    %1477 = arith.andi %1458, %1476 : i1 loc(#loc117)
    %1478 = arith.ori %1472, %1474 : i1
    %1479 = arith.andi %1477, %1478 : i1
    %1480 = arith.andi %1413, %1475 : i1
    %1481 = arith.andi %1471, %1463 : i1
    %1482 = arith.andi %1415, %1481 : i1
    %1483 = arith.andi %1413, %1473 : i1
    %1484 = arith.ori %1482, %1483 : i1
    %1485 = arith.select %1415, %1464, %false : i1
    %1486 = arith.andi %1471, %1469 : i1
    %1487 = arith.andi %1415, %1486 : i1
    %1488 = arith.andi %1473, %1480 : i1
    %1489 = arith.andi %1413, %1488 : i1
    %1490 = arith.ori %1487, %1489 : i1
    %1491 = arith.select %1415, %1470, %value_82 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1492 = arith.ori %1355, %1409 : i1
    %1493 = arith.ori %1357, %1406 : i1
    acir.path %1352 {branch = 38 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1352 {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1494 = arith.ori %1357, %1493 : i1 loc(#loc116)
    %value_84, %ready_85 = acir.read(%1352) {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1495 = arith.andi %1494, %ready_85 : i1 loc(#loc117)
    %1496 = arith.ori %1357, %1495 : i1 loc(#loc117)
    %1497 = arith.andi %1479, %1496 : i1 loc(#loc117)
    %1498 = arith.ori %1357, %1494 : i1
    %1499 = arith.andi %1497, %1498 : i1
    %1500 = arith.andi %1352, %1495 : i1
    %1501 = arith.andi %1492, %1484 : i1
    %1502 = arith.andi %1354, %1501 : i1
    %1503 = arith.andi %1352, %1493 : i1
    %1504 = arith.ori %1502, %1503 : i1
    %1505 = arith.select %1354, %1485, %false : i1
    %1506 = arith.andi %1492, %1490 : i1
    %1507 = arith.andi %1354, %1506 : i1
    %1508 = arith.andi %1493, %1500 : i1
    %1509 = arith.andi %1352, %1508 : i1
    %1510 = arith.ori %1507, %1509 : i1
    %1511 = arith.select %1354, %1491, %value_84 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1512 = arith.ori %1335, %1401 : i1
    %1513 = acir.unary %1505 "not" {acir.source_type = "bool"} : i1 -> i1
    %1514 = arith.andi %1512, %1504 : i1
    %1515 = arith.ori %1335, %1514 : i1
    %1516 = arith.andi %1499, %1515 : i1
    %1517 = arith.andi %1512, %1510 : i1
    %1518 = arith.andi %1330, %1517 : i1
    %1519 = arith.andi %1518, %1399 : i1
    %1520 = arith.andi %1516, %1346 : i1
    %1521 = arith.andi %1337, %1519 : i1
    %1522 = arith.andi %1330, %1521 : i1
    %1523 = acir.extract %1511 {acir.id = "v45", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc118)
    %1524 = arith.andi %1336, %1522 : i1
    %1525 = arith.ori %1335, %1524 : i1
    %1526 = arith.andi %1520, %1525 : i1
    %1527 = arith.andi %1330, %1523 : i1
    %1528 = arith.xori %1523, %true : i1
    %1529 = arith.andi %1330, %1528 : i1
    %1530 = arith.xori %1529, %true : i1
    %1531 = arith.ori %1530, %1524 : i1
    %1532 = arith.xori %1527, %true : i1
    %1533 = arith.ori %1532, %1524 : i1
    %1534 = acir.extract %1511 {acir.id = "v46", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
    %1535 = arith.andi %1529, %1531 : i1
    %1536 = arith.andi %1533, %1522 : i1
    %1537 = arith.andi %1527, %1536 : i1
    %1538 = arith.ori %1535, %1537 : i1
    %1539 = arith.select %1529, %false, %1534 : i1
    %1540 = arith.andi %1337, %1538 : i1
    %1541 = arith.ori %1335, %1540 : i1
    %1542 = arith.andi %1526, %1541 : i1
    %1543 = arith.andi %1330, %1539 : i1
    %1544 = arith.xori %1539, %true : i1
    %1545 = arith.andi %1330, %1544 : i1
    %1546 = arith.xori %1545, %true : i1
    %1547 = arith.ori %1546, %1540 : i1
    %1548 = arith.xori %1543, %true : i1
    %1549 = arith.ori %1548, %1540 : i1
    %1550 = acir.extract %1511 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc120)
    %1551 = arith.andi %1522, %1289 : i1 loc(#loc120)
    %1552 = arith.cmpi eq, %1550, %1290 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc120)
    %1553 = arith.andi %1545, %1547 : i1
    %1554 = arith.andi %1549, %1551 : i1
    %1555 = arith.andi %1543, %1554 : i1
    %1556 = arith.ori %1553, %1555 : i1
    %1557 = arith.select %1545, %false, %1552 : i1
    %1558 = arith.andi %1346, %1556 : i1
    %1559 = arith.ori %1335, %1558 : i1
    %1560 = arith.andi %1542, %1559 : i1
    %1561 = arith.andi %1330, %1557 : i1
    %1562 = arith.xori %1557, %true : i1
    %1563 = arith.andi %1330, %1562 : i1
    %1564 = arith.xori %1563, %true : i1
    %1565 = arith.ori %1564, %1558 : i1
    %1566 = arith.xori %1561, %true : i1
    %1567 = arith.ori %1566, %1558 : i1
    %1568 = acir.extract %1511 {acir.id = "v58", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc121)
    %1569 = acir.aggregate(%true, %1290, %1568) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc122)
    %1570 = arith.andi %1565, %1320 : i1
    %1571 = arith.andi %1563, %1570 : i1
    %1572 = arith.andi %1561, %1567 : i1
    %1573 = arith.ori %1571, %1572 : i1
    %1574 = arith.select %1563, %1321, %false : i1
    %1575 = arith.andi %1565, %1324 : i1
    %1576 = arith.andi %1563, %1575 : i1
    %1577 = arith.andi %1567, %1551 : i1
    %1578 = arith.andi %1561, %1577 : i1
    %1579 = arith.ori %1576, %1578 : i1
    %1580 = arith.select %1563, %1325, %1569 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1581 = arith.andi %1364, %1573 : i1
    %1582 = arith.ori %1335, %1581 : i1
    %1583 = arith.andi %1560, %1582 : i1
    %1584 = arith.andi %1330, %1574 : i1
    %1585 = arith.xori %1574, %true : i1
    %1586 = arith.andi %1330, %1585 : i1
    %1587 = arith.xori %1586, %true : i1
    %1588 = arith.ori %1587, %1581 : i1
    %1589 = arith.xori %1584, %true : i1
    %1590 = arith.ori %1589, %1581 : i1
    %1591 = acir.aggregate(%false, %1290, %c0_i32) {acir.id = "v70", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc123)
    %1592 = arith.andi %1588, %1573 : i1
    %1593 = arith.andi %1586, %1592 : i1
    %1594 = arith.andi %1584, %1590 : i1
    %1595 = arith.ori %1593, %1594 : i1
    %1596 = arith.select %1586, %1574, %false : i1
    %1597 = arith.andi %1588, %1579 : i1
    %1598 = arith.andi %1586, %1597 : i1
    %1599 = arith.andi %1590, %1289 : i1
    %1600 = arith.andi %1584, %1599 : i1
    %1601 = arith.ori %1598, %1600 : i1
    %1602 = arith.select %1586, %1580, %1591 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1603 = arith.andi %1334, %1320 : i1
    %1604 = arith.andi %1332, %1603 : i1
    %1605 = arith.andi %1380, %1595 : i1
    %1606 = arith.andi %1330, %1605 : i1
    %1607 = arith.ori %1604, %1606 : i1
    %1608 = arith.select %1332, %1321, %1596 : i1
    %1609 = arith.andi %1334, %1324 : i1
    %1610 = arith.andi %1332, %1609 : i1
    %1611 = arith.andi %1380, %1601 : i1
    %1612 = arith.andi %1330, %1611 : i1
    %1613 = arith.ori %1610, %1612 : i1
    %1614 = arith.select %1332, %1325, %1602 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1615 = arith.ori %1265, %1326 : i1
    %1616 = arith.andi %1280, %1272 : i1
    %1617 = arith.andi %1278, %1616 : i1
    %1618 = arith.andi %1615, %1607 : i1
    %1619 = arith.andi %1264, %1618 : i1
    %1620 = arith.ori %1617, %1619 : i1
    %1621 = arith.select %1278, %1264, %1608 : i1
    %1622 = arith.andi %1615, %1613 : i1
    %1623 = arith.andi %1264, %1622 : i1
    %1624 = arith.ori %1617, %1623 : i1
    %1625 = arith.select %1278, %1273, %1614 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1626 = arith.ori %1237, %1274 : i1
    %1627 = acir.unary %1621 "not" {acir.source_type = "bool"} : i1 -> i1
    %1628 = arith.andi %1626, %1620 : i1
    %1629 = arith.ori %1237, %1628 : i1
    %1630 = arith.andi %1583, %1629 : i1
    %1631 = arith.andi %1626, %1624 : i1
    %1632 = arith.andi %1236, %1631 : i1
    %1633 = acir.extract %value_68 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc124)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc125)
    %1634 = arith.cmpi ult, %1633, %c8_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc103)
    %1635 = arith.andi %1630, %1258 : i1 loc(#loc104)
    %1636 = arith.cmpi eq, %1633, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc105)
    %1637 = arith.andi %1635, %1258 : i1
    %1638 = arith.andi %1236, %1636 : i1
    %1639 = arith.cmpi ne, %1633, %c0_i32 : i32 loc(#loc105)
    %1640 = arith.andi %1236, %1639 : i1
    %1641 = arith.xori %1640, %true : i1 loc(#loc106)
    %1642 = arith.ori %1641, %1257 : i1
    %1643 = arith.xori %1638, %true : i1
    %1644 = arith.ori %1643, %1257 : i1
    %1645 = arith.andi %1640, %1642 : i1
    %1646 = arith.andi %1638, %1644 : i1
    %1647 = arith.ori %1645, %1646 : i1
    %1648 = arith.select %1640, %1255, %1269 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1649 = arith.andi %1274, %1647 : i1
    %1650 = arith.ori %1237, %1649 : i1
    %1651 = arith.andi %1637, %1650 : i1
    %1652 = arith.andi %1236, %1641 : i1
    %1653 = arith.xori %1652, %true : i1
    %1654 = arith.ori %1653, %1649 : i1
    %1655 = arith.ori %1641, %1649 : i1
    acir.path %1640 {branch = 39 : i64, kind = "branch", txn = 6 : i64} loc(#loc106)
    %value_86, %ready_87 = acir.read(%1640) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc106)
    %1656 = arith.andi %1655, %ready_87 : i1 loc(#loc106)
    %1657 = arith.ori %1641, %1656 : i1 loc(#loc106)
    %1658 = arith.andi %1651, %1657 : i1 loc(#loc106)
    %1659 = arith.andi %ready_87, %1254 : i1 loc(#loc106)
    %1660 = arith.extui %1633 : i32 to i64 loc(#loc106)
    %1661 = arith.cmpi ult, %1660, %c8_i64 : i64 loc(#loc106)
    %1662 = arith.select %1661, %1660, %c0_i64 : i64 loc(#loc106)
    %1663 = arith.andi %1659, %1661 : i1 loc(#loc106)
    %1664 = "acir.array.index"(%value_86, %1662) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc106)
    %value_88, %ready_89 = acir.read(%1640) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc109)
    %1665 = arith.andi %1655, %ready_89 : i1 loc(#loc109)
    %1666 = arith.ori %1641, %1665 : i1 loc(#loc109)
    %1667 = arith.andi %1658, %1666 : i1 loc(#loc109)
    %1668 = arith.andi %ready_89, %1254 : i1 loc(#loc109)
    %1669 = arith.andi %1668, %1661 : i1 loc(#loc109)
    %1670 = "acir.array.index"(%value_88, %1662) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc109)
    %1671 = arith.andi %1663, %1669 : i1 loc(#loc110)
    %1672 = arith.cmpi eq, %1664, %1670 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc110)
    %1673 = arith.andi %1655, %1671 : i1
    %1674 = arith.ori %1641, %1673 : i1
    %1675 = arith.andi %1667, %1674 : i1
    %1676 = arith.andi %1640, %1672 : i1
    %1677 = arith.cmpi ne, %1664, %1670 : i32 loc(#loc110)
    %1678 = arith.andi %1640, %1677 : i1
    %1679 = arith.xori %1678, %true : i1
    %1680 = arith.ori %1679, %1673 : i1
    %1681 = arith.xori %1676, %true : i1 loc(#loc111)
    %1682 = arith.ori %1681, %1673 : i1
    acir.path %1676 {branch = 40 : i64, kind = "branch", txn = 6 : i64} loc(#loc111)
    %value_90, %ready_91 = acir.read(%1676) {branch = 40 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc111)
    %1683 = arith.andi %1682, %ready_91 : i1 loc(#loc111)
    %1684 = arith.ori %1681, %1683 : i1 loc(#loc111)
    %1685 = arith.andi %1675, %1684 : i1 loc(#loc111)
    %1686 = arith.andi %ready_91, %1254 : i1 loc(#loc111)
    %1687 = arith.andi %1686, %1661 : i1 loc(#loc111)
    %1688 = "acir.array.index"(%value_90, %1662) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc111)
    %1689 = arith.andi %1663, %1687 : i1 loc(#loc112)
    %1690 = acir.aggregate(%true, %1664, %1688) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc112)
    %1691 = arith.andi %1680, %1647 : i1
    %1692 = arith.andi %1678, %1691 : i1
    %1693 = arith.andi %1676, %1682 : i1
    %1694 = arith.ori %1692, %1693 : i1
    %1695 = arith.select %1678, %1640, %false : i1
    %1696 = arith.andi %1682, %1689 : i1
    %1697 = arith.andi %1676, %1696 : i1
    %1698 = arith.ori %1692, %1697 : i1
    %1699 = arith.select %1678, %1648, %1690 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1700 = arith.ori %1641, %1655 : i1
    %1701 = arith.andi %1700, %1694 : i1
    %1702 = arith.ori %1641, %1701 : i1
    %1703 = arith.andi %1685, %1702 : i1
    %1704 = arith.andi %1640, %1695 : i1
    %1705 = arith.xori %1695, %true : i1
    %1706 = arith.andi %1640, %1705 : i1
    %1707 = arith.xori %1706, %true : i1
    %1708 = arith.ori %1707, %1701 : i1
    %1709 = arith.xori %1704, %true : i1 loc(#loc113)
    %1710 = arith.ori %1709, %1701 : i1
    acir.path %1704 {branch = 41 : i64, kind = "branch", txn = 6 : i64} loc(#loc113)
    acir.path %1704 {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc113)
    %1711 = arith.ori %1709, %1710 : i1 loc(#loc113)
    %1712 = arith.andi %1710, %1663 : i1 loc(#loc113)
    %1713 = arith.andi %1704, %1712 : i1 loc(#loc113)
    %1714 = arith.subi %1664, %c1_i32 {acir.id = "v20", acir.source_type = "u32"} : i32 loc(#loc114)
    %1715 = arith.andi %1714, %c3_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc115)
    %1716 = arith.cmpi eq, %1715, %c0_i32 {acir.id = "v24", acir.source_type = "bool"} : i32 loc(#loc116)
    %1717 = arith.cmpi eq, %1715, %c1_i32 {acir.id = "v26", acir.source_type = "bool"} : i32 loc(#loc116)
    %1718 = arith.cmpi eq, %1715, %c2_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc116)
    %1719 = arith.cmpi eq, %1715, %c3_i32 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc116)
    acir.path %1704 {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1720 = arith.ori %1709, %1711 : i1 loc(#loc116)
    %1721 = arith.andi %1711, %1713 : i1 loc(#loc116)
    %1722 = arith.andi %1704, %1721 : i1 loc(#loc116)
    %1723 = arith.andi %1720, %1722 : i1
    %1724 = arith.ori %1709, %1723 : i1
    %1725 = arith.andi %1703, %1724 : i1
    %1726 = arith.andi %1704, %1716 : i1
    %1727 = arith.cmpi ne, %1715, %c0_i32 : i32 loc(#loc116)
    %1728 = arith.andi %1704, %1727 : i1
    %1729 = arith.xori %1728, %true : i1
    %1730 = arith.ori %1729, %1723 : i1
    %1731 = arith.xori %1726, %true : i1 loc(#loc116)
    %1732 = arith.ori %1731, %1723 : i1
    %1733 = arith.andi %1730, %1722 : i1
    %1734 = arith.andi %1728, %1733 : i1
    %1735 = arith.andi %1726, %1732 : i1
    %1736 = arith.ori %1734, %1735 : i1
    %1737 = arith.select %1728, %1717, %true : i1
    %1738 = arith.ori %1709, %1720 : i1
    %1739 = arith.andi %1738, %1736 : i1
    %1740 = arith.ori %1709, %1739 : i1
    %1741 = arith.andi %1725, %1740 : i1
    %1742 = arith.andi %1704, %1737 : i1
    %1743 = arith.xori %1737, %true : i1
    %1744 = arith.andi %1704, %1743 : i1
    %1745 = arith.xori %1744, %true : i1
    %1746 = arith.ori %1745, %1739 : i1
    %1747 = arith.xori %1742, %true : i1
    %1748 = arith.ori %1747, %1739 : i1
    %1749 = arith.andi %1746, %1722 : i1
    %1750 = arith.andi %1744, %1749 : i1
    %1751 = arith.andi %1742, %1748 : i1
    %1752 = arith.ori %1750, %1751 : i1
    %1753 = arith.select %1744, %1718, %true : i1
    %1754 = arith.ori %1709, %1738 : i1
    %1755 = arith.andi %1754, %1752 : i1
    %1756 = arith.ori %1709, %1755 : i1
    %1757 = arith.andi %1741, %1756 : i1
    %1758 = arith.andi %1704, %1753 : i1
    %1759 = arith.xori %1753, %true : i1
    %1760 = arith.andi %1704, %1759 : i1
    %1761 = arith.xori %1760, %true : i1
    %1762 = arith.ori %1761, %1755 : i1
    %1763 = arith.xori %1758, %true : i1
    %1764 = arith.ori %1763, %1755 : i1
    %1765 = arith.andi %1762, %1722 : i1
    %1766 = arith.andi %1760, %1765 : i1
    %1767 = arith.andi %1758, %1764 : i1
    %1768 = arith.ori %1766, %1767 : i1
    %1769 = arith.select %1760, %1719, %true : i1
    %1770 = arith.ori %1709, %1754 : i1
    %1771 = arith.andi %1770, %1768 : i1 loc(#loc116)
    %1772 = arith.andi %1704, %1771 : i1 loc(#loc116)
    %1773 = arith.andi %1772, %1769 : i1 loc(#loc116)
    %1774 = arith.andi %1711, %1773 : i1 loc(#loc116)
    %1775 = arith.ori %1709, %1774 : i1 loc(#loc116)
    %1776 = arith.andi %1757, %1775 : i1 loc(#loc116)
    %1777 = arith.andi %1774, %1713 : i1 loc(#loc116)
    %1778 = arith.andi %1704, %1777 : i1 loc(#loc116)
    %1779 = arith.andi %1775, %1778 : i1
    %1780 = arith.ori %1709, %1779 : i1
    %1781 = arith.andi %1776, %1780 : i1
    %1782 = arith.ori %1729, %1779 : i1
    %1783 = arith.andi %1782, %1778 : i1
    %1784 = arith.ori %1729, %1783 : i1
    %1785 = arith.andi %1781, %1784 : i1
    %1786 = arith.andi %1728, %1717 : i1
    %1787 = arith.cmpi ne, %1715, %c1_i32 : i32 loc(#loc116)
    %1788 = arith.andi %1728, %1787 : i1
    %1789 = arith.xori %1788, %true : i1
    %1790 = arith.ori %1789, %1783 : i1
    %1791 = arith.andi %1790, %1778 : i1
    %1792 = arith.ori %1789, %1791 : i1
    %1793 = arith.andi %1785, %1792 : i1
    %1794 = arith.andi %1788, %1718 : i1
    %1795 = arith.cmpi ne, %1715, %c2_i32 : i32 loc(#loc116)
    %1796 = arith.andi %1788, %1795 : i1
    %1797 = arith.xori %1796, %true : i1
    %1798 = arith.ori %1797, %1791 : i1
    %1799 = arith.andi %1798, %1778 : i1
    %1800 = arith.ori %1797, %1799 : i1
    %1801 = arith.andi %1793, %1800 : i1
    %1802 = arith.andi %1796, %1719 : i1
    %1803 = arith.cmpi ne, %1715, %c3_i32 : i32 loc(#loc116)
    %1804 = arith.andi %1796, %1803 : i1
    %1805 = arith.xori %1804, %true : i1
    %1806 = arith.ori %1805, %1799 : i1
    %1807 = arith.xori %1802, %true : i1 loc(#loc116)
    %1808 = arith.ori %1807, %1799 : i1
    acir.path %1802 {branch = 42 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1802 {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1809 = arith.ori %1807, %1808 : i1 loc(#loc116)
    %value_92, %ready_93 = acir.read(%1802) {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1810 = arith.andi %1809, %ready_93 : i1 loc(#loc117)
    %1811 = arith.ori %1807, %1810 : i1 loc(#loc117)
    %1812 = arith.andi %1801, %1811 : i1 loc(#loc117)
    %1813 = arith.ori %1807, %1809 : i1
    %1814 = arith.andi %1812, %1813 : i1
    %1815 = arith.andi %1802, %1810 : i1
    %1816 = arith.andi %1804, %1806 : i1
    %1817 = arith.andi %1802, %1808 : i1
    %1818 = arith.ori %1816, %1817 : i1
    %1819 = arith.andi %1808, %1815 : i1
    %1820 = arith.andi %1802, %1819 : i1
    %1821 = arith.ori %1816, %1820 : i1
    %1822 = arith.select %1804, %1405, %value_92 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1823 = arith.ori %1797, %1798 : i1
    %1824 = arith.xori %1794, %true : i1 loc(#loc116)
    %1825 = arith.ori %1824, %1791 : i1
    acir.path %1794 {branch = 43 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1794 {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1826 = arith.ori %1824, %1825 : i1 loc(#loc116)
    %value_94, %ready_95 = acir.read(%1794) {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1827 = arith.andi %1826, %ready_95 : i1 loc(#loc117)
    %1828 = arith.ori %1824, %1827 : i1 loc(#loc117)
    %1829 = arith.andi %1814, %1828 : i1 loc(#loc117)
    %1830 = arith.ori %1824, %1826 : i1
    %1831 = arith.andi %1829, %1830 : i1
    %1832 = arith.andi %1794, %1827 : i1
    %1833 = arith.andi %1823, %1818 : i1
    %1834 = arith.andi %1796, %1833 : i1
    %1835 = arith.andi %1794, %1825 : i1
    %1836 = arith.ori %1834, %1835 : i1
    %1837 = arith.select %1796, %1804, %false : i1
    %1838 = arith.andi %1823, %1821 : i1
    %1839 = arith.andi %1796, %1838 : i1
    %1840 = arith.andi %1825, %1832 : i1
    %1841 = arith.andi %1794, %1840 : i1
    %1842 = arith.ori %1839, %1841 : i1
    %1843 = arith.select %1796, %1822, %value_94 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1844 = arith.ori %1789, %1790 : i1
    %1845 = arith.xori %1786, %true : i1 loc(#loc116)
    %1846 = arith.ori %1845, %1783 : i1
    acir.path %1786 {branch = 44 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1786 {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1847 = arith.ori %1845, %1846 : i1 loc(#loc116)
    %value_96, %ready_97 = acir.read(%1786) {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1848 = arith.andi %1847, %ready_97 : i1 loc(#loc117)
    %1849 = arith.ori %1845, %1848 : i1 loc(#loc117)
    %1850 = arith.andi %1831, %1849 : i1 loc(#loc117)
    %1851 = arith.ori %1845, %1847 : i1
    %1852 = arith.andi %1850, %1851 : i1
    %1853 = arith.andi %1786, %1848 : i1
    %1854 = arith.andi %1844, %1836 : i1
    %1855 = arith.andi %1788, %1854 : i1
    %1856 = arith.andi %1786, %1846 : i1
    %1857 = arith.ori %1855, %1856 : i1
    %1858 = arith.select %1788, %1837, %false : i1
    %1859 = arith.andi %1844, %1842 : i1
    %1860 = arith.andi %1788, %1859 : i1
    %1861 = arith.andi %1846, %1853 : i1
    %1862 = arith.andi %1786, %1861 : i1
    %1863 = arith.ori %1860, %1862 : i1
    %1864 = arith.select %1788, %1843, %value_96 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1865 = arith.ori %1729, %1782 : i1
    %1866 = arith.ori %1731, %1779 : i1
    acir.path %1726 {branch = 45 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    acir.path %1726 {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc116)
    %1867 = arith.ori %1731, %1866 : i1 loc(#loc116)
    %value_98, %ready_99 = acir.read(%1726) {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc117)
    %1868 = arith.andi %1867, %ready_99 : i1 loc(#loc117)
    %1869 = arith.ori %1731, %1868 : i1 loc(#loc117)
    %1870 = arith.andi %1852, %1869 : i1 loc(#loc117)
    %1871 = arith.ori %1731, %1867 : i1
    %1872 = arith.andi %1870, %1871 : i1
    %1873 = arith.andi %1726, %1868 : i1
    %1874 = arith.andi %1865, %1857 : i1
    %1875 = arith.andi %1728, %1874 : i1
    %1876 = arith.andi %1726, %1866 : i1
    %1877 = arith.ori %1875, %1876 : i1
    %1878 = arith.select %1728, %1858, %false : i1
    %1879 = arith.andi %1865, %1863 : i1
    %1880 = arith.andi %1728, %1879 : i1
    %1881 = arith.andi %1866, %1873 : i1
    %1882 = arith.andi %1726, %1881 : i1
    %1883 = arith.ori %1880, %1882 : i1
    %1884 = arith.select %1728, %1864, %value_98 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1885 = arith.ori %1709, %1775 : i1
    %1886 = acir.unary %1878 "not" {acir.source_type = "bool"} : i1 -> i1
    %1887 = arith.andi %1885, %1877 : i1
    %1888 = arith.ori %1709, %1887 : i1
    %1889 = arith.andi %1872, %1888 : i1
    %1890 = arith.andi %1885, %1883 : i1
    %1891 = arith.andi %1704, %1890 : i1
    %1892 = arith.andi %1891, %1773 : i1
    %1893 = arith.andi %1889, %1720 : i1
    %1894 = arith.andi %1711, %1892 : i1
    %1895 = arith.andi %1704, %1894 : i1
    %1896 = acir.extract %1884 {acir.id = "v45", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc118)
    %1897 = arith.andi %1710, %1895 : i1
    %1898 = arith.ori %1709, %1897 : i1
    %1899 = arith.andi %1893, %1898 : i1
    %1900 = arith.andi %1704, %1896 : i1
    %1901 = arith.xori %1896, %true : i1
    %1902 = arith.andi %1704, %1901 : i1
    %1903 = arith.xori %1902, %true : i1
    %1904 = arith.ori %1903, %1897 : i1
    %1905 = arith.xori %1900, %true : i1
    %1906 = arith.ori %1905, %1897 : i1
    %1907 = acir.extract %1884 {acir.id = "v46", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
    %1908 = arith.andi %1902, %1904 : i1
    %1909 = arith.andi %1906, %1895 : i1
    %1910 = arith.andi %1900, %1909 : i1
    %1911 = arith.ori %1908, %1910 : i1
    %1912 = arith.select %1902, %false, %1907 : i1
    %1913 = arith.andi %1711, %1911 : i1
    %1914 = arith.ori %1709, %1913 : i1
    %1915 = arith.andi %1899, %1914 : i1
    %1916 = arith.andi %1704, %1912 : i1
    %1917 = arith.xori %1912, %true : i1
    %1918 = arith.andi %1704, %1917 : i1
    %1919 = arith.xori %1918, %true : i1
    %1920 = arith.ori %1919, %1913 : i1
    %1921 = arith.xori %1916, %true : i1
    %1922 = arith.ori %1921, %1913 : i1
    %1923 = acir.extract %1884 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc120)
    %1924 = arith.andi %1895, %1663 : i1 loc(#loc120)
    %1925 = arith.cmpi eq, %1923, %1664 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc120)
    %1926 = arith.andi %1918, %1920 : i1
    %1927 = arith.andi %1922, %1924 : i1
    %1928 = arith.andi %1916, %1927 : i1
    %1929 = arith.ori %1926, %1928 : i1
    %1930 = arith.select %1918, %false, %1925 : i1
    %1931 = arith.andi %1720, %1929 : i1
    %1932 = arith.ori %1709, %1931 : i1
    %1933 = arith.andi %1915, %1932 : i1
    %1934 = arith.andi %1704, %1930 : i1
    %1935 = arith.xori %1930, %true : i1
    %1936 = arith.andi %1704, %1935 : i1
    %1937 = arith.xori %1936, %true : i1
    %1938 = arith.ori %1937, %1931 : i1
    %1939 = arith.xori %1934, %true : i1
    %1940 = arith.ori %1939, %1931 : i1
    %1941 = acir.extract %1884 {acir.id = "v58", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc121)
    %1942 = acir.aggregate(%true, %1664, %1941) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc122)
    %1943 = arith.andi %1938, %1694 : i1
    %1944 = arith.andi %1936, %1943 : i1
    %1945 = arith.andi %1934, %1940 : i1
    %1946 = arith.ori %1944, %1945 : i1
    %1947 = arith.select %1936, %1695, %false : i1
    %1948 = arith.andi %1938, %1698 : i1
    %1949 = arith.andi %1936, %1948 : i1
    %1950 = arith.andi %1940, %1924 : i1
    %1951 = arith.andi %1934, %1950 : i1
    %1952 = arith.ori %1949, %1951 : i1
    %1953 = arith.select %1936, %1699, %1942 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1954 = arith.andi %1738, %1946 : i1
    %1955 = arith.ori %1709, %1954 : i1
    %1956 = arith.andi %1933, %1955 : i1
    %1957 = arith.andi %1704, %1947 : i1
    %1958 = arith.xori %1947, %true : i1
    %1959 = arith.andi %1704, %1958 : i1
    %1960 = arith.xori %1959, %true : i1
    %1961 = arith.ori %1960, %1954 : i1
    %1962 = arith.xori %1957, %true : i1
    %1963 = arith.ori %1962, %1954 : i1
    %1964 = acir.aggregate(%false, %1664, %c0_i32) {acir.id = "v70", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc123)
    %1965 = arith.andi %1961, %1946 : i1
    %1966 = arith.andi %1959, %1965 : i1
    %1967 = arith.andi %1957, %1963 : i1
    %1968 = arith.ori %1966, %1967 : i1
    %1969 = arith.select %1959, %1947, %false : i1
    %1970 = arith.andi %1961, %1952 : i1
    %1971 = arith.andi %1959, %1970 : i1
    %1972 = arith.andi %1963, %1663 : i1
    %1973 = arith.andi %1957, %1972 : i1
    %1974 = arith.ori %1971, %1973 : i1
    %1975 = arith.select %1959, %1953, %1964 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1976 = arith.andi %1708, %1694 : i1
    %1977 = arith.andi %1706, %1976 : i1
    %1978 = arith.andi %1754, %1968 : i1
    %1979 = arith.andi %1704, %1978 : i1
    %1980 = arith.ori %1977, %1979 : i1
    %1981 = arith.select %1706, %1695, %1969 : i1
    %1982 = arith.andi %1708, %1698 : i1
    %1983 = arith.andi %1706, %1982 : i1
    %1984 = arith.andi %1754, %1974 : i1
    %1985 = arith.andi %1704, %1984 : i1
    %1986 = arith.ori %1983, %1985 : i1
    %1987 = arith.select %1706, %1699, %1975 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1988 = arith.ori %1641, %1700 : i1
    %1989 = arith.andi %1654, %1647 : i1
    %1990 = arith.andi %1652, %1989 : i1
    %1991 = arith.andi %1988, %1980 : i1
    %1992 = arith.andi %1640, %1991 : i1
    %1993 = arith.ori %1990, %1992 : i1
    %1994 = arith.select %1652, %1640, %1981 : i1
    %1995 = arith.andi %1988, %1986 : i1
    %1996 = arith.andi %1640, %1995 : i1
    %1997 = arith.ori %1990, %1996 : i1
    %1998 = arith.select %1652, %1648, %1987 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1999 = acir.unary %1994 "not" {acir.source_type = "bool"} : i1 -> i1
    %2000 = arith.andi %1626, %1993 : i1
    %2001 = arith.ori %1237, %2000 : i1
    %2002 = arith.andi %1956, %2001 : i1
    %2003 = arith.andi %1626, %1997 : i1
    %2004 = arith.andi %1236, %2003 : i1
    %2005 = acir.extract %value_68 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc126)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc108)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc108)
    %value_100, %ready_101 = acir.read(%1236) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc127)
    %2006 = arith.andi %1274, %ready_101 : i1 loc(#loc127)
    %2007 = arith.ori %1237, %2006 : i1 loc(#loc127)
    %2008 = arith.andi %2002, %2007 : i1 loc(#loc127)
    %value_102, %ready_103 = acir.read(%1236) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc128)
    %2009 = arith.andi %1274, %ready_103 : i1 loc(#loc128)
    %2010 = arith.ori %1237, %2009 : i1 loc(#loc128)
    %2011 = arith.andi %2008, %2010 : i1 loc(#loc128)
    %2012 = arith.andi %ready_101, %ready_103 : i1 loc(#loc127)
    %2013 = arith.subi %value_100, %value_102 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc127)
    %2014 = arith.cmpi ult, %2013, %c4_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc127)
    %2015 = arith.andi %2011, %1626 : i1
    %2016 = arith.andi %1274, %2012 : i1
    %2017 = arith.andi %1236, %2016 : i1
    %2018 = arith.andi %1253, %2017 : i1 loc(#loc129)
    %2019 = arith.andi %1236, %2018 : i1 loc(#loc129)
    %2020 = arith.andi %2019, %2014 : i1 loc(#loc129)
    %2021 = arith.andi %1240, %2020 : i1 loc(#loc129)
    %2022 = arith.ori %1237, %2021 : i1 loc(#loc129)
    %2023 = arith.andi %2015, %2022 : i1 loc(#loc129)
    %2024 = arith.andi %2021, %ready_69 : i1 loc(#loc129)
    %2025 = arith.andi %1236, %2024 : i1 loc(#loc129)
    %value_104, %ready_105 = acir.read(%1236) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc130)
    %2026 = arith.andi %2022, %ready_105 : i1 loc(#loc130)
    %2027 = arith.ori %1237, %2026 : i1 loc(#loc130)
    %2028 = arith.andi %2023, %2027 : i1 loc(#loc130)
    %2029 = arith.subi %value_104, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc131)
    %2030 = arith.andi %2029, %c3_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc132)
    %2031 = arith.cmpi eq, %2030, %c0_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc133)
    %2032 = arith.cmpi eq, %2030, %c1_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc133)
    %2033 = arith.cmpi eq, %2030, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc133)
    %2034 = arith.cmpi eq, %2030, %c3_i32 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc133)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2035 = arith.ori %1237, %2022 : i1 loc(#loc133)
    %2036 = arith.andi %1236, %2026 : i1 loc(#loc133)
    %2037 = arith.andi %2035, %2036 : i1
    %2038 = arith.ori %1237, %2037 : i1
    %2039 = arith.andi %2028, %2038 : i1
    %2040 = arith.andi %1236, %2031 : i1
    %2041 = arith.cmpi ne, %2030, %c0_i32 : i32 loc(#loc133)
    %2042 = arith.andi %1236, %2041 : i1
    %2043 = arith.xori %2042, %true : i1
    %2044 = arith.ori %2043, %2037 : i1
    %2045 = arith.xori %2040, %true : i1 loc(#loc133)
    %2046 = arith.ori %2045, %2037 : i1
    %2047 = arith.andi %2044, %2036 : i1
    %2048 = arith.andi %2042, %2047 : i1
    %2049 = arith.andi %2040, %2046 : i1
    %2050 = arith.ori %2048, %2049 : i1
    %2051 = arith.select %2042, %2032, %true : i1
    %2052 = arith.ori %1237, %2035 : i1
    %2053 = arith.andi %2052, %2050 : i1
    %2054 = arith.ori %1237, %2053 : i1
    %2055 = arith.andi %2039, %2054 : i1
    %2056 = arith.andi %1236, %2051 : i1
    %2057 = arith.xori %2051, %true : i1
    %2058 = arith.andi %1236, %2057 : i1
    %2059 = arith.xori %2058, %true : i1
    %2060 = arith.ori %2059, %2053 : i1
    %2061 = arith.xori %2056, %true : i1
    %2062 = arith.ori %2061, %2053 : i1
    %2063 = arith.andi %2060, %2036 : i1
    %2064 = arith.andi %2058, %2063 : i1
    %2065 = arith.andi %2056, %2062 : i1
    %2066 = arith.ori %2064, %2065 : i1
    %2067 = arith.select %2058, %2033, %true : i1
    %2068 = arith.ori %1237, %2052 : i1
    %2069 = arith.andi %2068, %2066 : i1
    %2070 = arith.ori %1237, %2069 : i1
    %2071 = arith.andi %2055, %2070 : i1
    %2072 = arith.andi %1236, %2067 : i1
    %2073 = arith.xori %2067, %true : i1
    %2074 = arith.andi %1236, %2073 : i1
    %2075 = arith.xori %2074, %true : i1
    %2076 = arith.ori %2075, %2069 : i1
    %2077 = arith.xori %2072, %true : i1
    %2078 = arith.ori %2077, %2069 : i1
    %2079 = arith.andi %2076, %2036 : i1
    %2080 = arith.andi %2074, %2079 : i1
    %2081 = arith.andi %2072, %2078 : i1
    %2082 = arith.ori %2080, %2081 : i1
    %2083 = arith.select %2074, %2034, %true : i1
    %2084 = arith.ori %1237, %2068 : i1
    %2085 = arith.andi %2084, %2082 : i1 loc(#loc133)
    %2086 = arith.andi %1236, %2085 : i1 loc(#loc133)
    %2087 = arith.andi %2086, %2083 : i1 loc(#loc133)
    %2088 = arith.andi %2022, %2087 : i1 loc(#loc133)
    %2089 = arith.ori %1237, %2088 : i1 loc(#loc133)
    %2090 = arith.andi %2071, %2089 : i1 loc(#loc133)
    %2091 = arith.andi %2088, %ready_105 : i1 loc(#loc133)
    %2092 = arith.andi %1236, %2091 : i1 loc(#loc133)
    %2093 = arith.andi %2088, %2025 : i1 loc(#loc133)
    %2094 = arith.andi %1236, %2093 : i1 loc(#loc133)
    %2095 = arith.andi %2089, %2092 : i1
    %2096 = arith.ori %1237, %2095 : i1
    %2097 = arith.andi %2090, %2096 : i1
    %2098 = arith.ori %2043, %2095 : i1
    %2099 = arith.andi %2098, %2092 : i1
    %2100 = arith.ori %2043, %2099 : i1
    %2101 = arith.andi %2097, %2100 : i1
    %2102 = arith.andi %2042, %2032 : i1
    %2103 = arith.cmpi ne, %2030, %c1_i32 : i32 loc(#loc133)
    %2104 = arith.andi %2042, %2103 : i1
    %2105 = arith.xori %2104, %true : i1
    %2106 = arith.ori %2105, %2099 : i1
    %2107 = arith.andi %2106, %2092 : i1
    %2108 = arith.ori %2105, %2107 : i1
    %2109 = arith.andi %2101, %2108 : i1
    %2110 = arith.andi %2104, %2033 : i1
    %2111 = arith.cmpi ne, %2030, %c2_i32 : i32 loc(#loc133)
    %2112 = arith.andi %2104, %2111 : i1
    %2113 = arith.xori %2112, %true : i1
    %2114 = arith.ori %2113, %2107 : i1
    %2115 = arith.andi %2114, %2092 : i1
    %2116 = arith.ori %2113, %2115 : i1
    %2117 = arith.andi %2109, %2116 : i1
    %2118 = arith.andi %2112, %2034 : i1
    %2119 = arith.xori %2118, %true : i1 loc(#loc133)
    %2120 = arith.ori %2119, %2115 : i1
    acir.path %2118 {branch = 46 : i64, kind = "branch", txn = 6 : i64} loc(#loc133)
    acir.path %2118 {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2121 = arith.ori %2119, %2120 : i1 loc(#loc133)
    acir.path %2118 {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate", "top.rob.slots.3:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2122 = arith.ori %2119, %2121 : i1 loc(#loc133)
    %value_106, %ready_107 = acir.read(%2118) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate", "top.rob.slots.3:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc134)
    %2123 = arith.andi %2122, %ready_107 : i1 loc(#loc134)
    %2124 = arith.ori %2119, %2123 : i1 loc(#loc134)
    %2125 = arith.andi %2117, %2124 : i1 loc(#loc134)
    %2126 = acir.extract %value_106 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
    %2127 = acir.unary %2126 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
    %2128 = arith.ori %2119, %2122 : i1
    %2129 = arith.andi %2125, %2128 : i1
    %2130 = arith.andi %2118, %2123 : i1
    %2131 = arith.andi %2121, %2130 : i1 loc(#loc136)
    %2132 = arith.andi %2118, %2131 : i1 loc(#loc136)
    %2133 = arith.andi %2132, %2127 : i1 loc(#loc136)
    %2134 = arith.andi %2120, %2133 : i1 loc(#loc136)
    %2135 = arith.ori %2119, %2134 : i1 loc(#loc136)
    %2136 = arith.andi %2129, %2135 : i1 loc(#loc136)
    %2137 = arith.andi %2134, %2092 : i1 loc(#loc136)
    %2138 = arith.andi %2118, %2137 : i1 loc(#loc136)
    %2139 = arith.andi %2134, %2094 : i1 loc(#loc136)
    %2140 = arith.andi %2118, %2139 : i1 loc(#loc136)
    %2141 = arith.andi %2138, %2140 : i1 loc(#loc137)
    %2142 = acir.aggregate(%true, %false, %value_104, %2005, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc137)
    %2143 = arith.andi %2135, %2141 : i1 loc(#loc138)
    %2144 = arith.ori %2119, %2143 : i1 loc(#loc138)
    %2145 = arith.andi %2136, %2144 : i1 loc(#loc138)
    %2146 = arith.xori %2110, %true : i1 loc(#loc133)
    %2147 = arith.ori %2146, %2107 : i1
    acir.path %2110 {branch = 47 : i64, kind = "branch", txn = 6 : i64} loc(#loc133)
    acir.path %2110 {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2148 = arith.ori %2146, %2147 : i1 loc(#loc133)
    acir.path %2110 {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate", "top.rob.slots.2:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2149 = arith.ori %2146, %2148 : i1 loc(#loc133)
    %value_108, %ready_109 = acir.read(%2110) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate", "top.rob.slots.2:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc134)
    %2150 = arith.andi %2149, %ready_109 : i1 loc(#loc134)
    %2151 = arith.ori %2146, %2150 : i1 loc(#loc134)
    %2152 = arith.andi %2145, %2151 : i1 loc(#loc134)
    %2153 = acir.extract %value_108 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
    %2154 = acir.unary %2153 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
    %2155 = arith.ori %2146, %2149 : i1
    %2156 = arith.andi %2152, %2155 : i1
    %2157 = arith.andi %2110, %2150 : i1
    %2158 = arith.andi %2148, %2157 : i1 loc(#loc136)
    %2159 = arith.andi %2110, %2158 : i1 loc(#loc136)
    %2160 = arith.andi %2159, %2154 : i1 loc(#loc136)
    %2161 = arith.andi %2147, %2160 : i1 loc(#loc136)
    %2162 = arith.ori %2146, %2161 : i1 loc(#loc136)
    %2163 = arith.andi %2156, %2162 : i1 loc(#loc136)
    %2164 = arith.andi %2161, %2092 : i1 loc(#loc136)
    %2165 = arith.andi %2110, %2164 : i1 loc(#loc136)
    %2166 = arith.andi %2161, %2094 : i1 loc(#loc136)
    %2167 = arith.andi %2110, %2166 : i1 loc(#loc136)
    %2168 = arith.andi %2165, %2167 : i1 loc(#loc137)
    %2169 = arith.andi %2162, %2168 : i1 loc(#loc138)
    %2170 = arith.ori %2146, %2169 : i1 loc(#loc138)
    %2171 = arith.andi %2163, %2170 : i1 loc(#loc138)
    %2172 = arith.xori %2102, %true : i1 loc(#loc133)
    %2173 = arith.ori %2172, %2099 : i1
    acir.path %2102 {branch = 48 : i64, kind = "branch", txn = 6 : i64} loc(#loc133)
    acir.path %2102 {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2174 = arith.ori %2172, %2173 : i1 loc(#loc133)
    acir.path %2102 {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate", "top.rob.slots.1:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2175 = arith.ori %2172, %2174 : i1 loc(#loc133)
    %value_110, %ready_111 = acir.read(%2102) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate", "top.rob.slots.1:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc134)
    %2176 = arith.andi %2175, %ready_111 : i1 loc(#loc134)
    %2177 = arith.ori %2172, %2176 : i1 loc(#loc134)
    %2178 = arith.andi %2171, %2177 : i1 loc(#loc134)
    %2179 = acir.extract %value_110 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
    %2180 = acir.unary %2179 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
    %2181 = arith.ori %2172, %2175 : i1
    %2182 = arith.andi %2178, %2181 : i1
    %2183 = arith.andi %2102, %2176 : i1
    %2184 = arith.andi %2174, %2183 : i1 loc(#loc136)
    %2185 = arith.andi %2102, %2184 : i1 loc(#loc136)
    %2186 = arith.andi %2185, %2180 : i1 loc(#loc136)
    %2187 = arith.andi %2173, %2186 : i1 loc(#loc136)
    %2188 = arith.ori %2172, %2187 : i1 loc(#loc136)
    %2189 = arith.andi %2182, %2188 : i1 loc(#loc136)
    %2190 = arith.andi %2187, %2092 : i1 loc(#loc136)
    %2191 = arith.andi %2102, %2190 : i1 loc(#loc136)
    %2192 = arith.andi %2187, %2094 : i1 loc(#loc136)
    %2193 = arith.andi %2102, %2192 : i1 loc(#loc136)
    %2194 = arith.andi %2191, %2193 : i1 loc(#loc137)
    %2195 = arith.andi %2188, %2194 : i1 loc(#loc138)
    %2196 = arith.ori %2172, %2195 : i1 loc(#loc138)
    %2197 = arith.andi %2189, %2196 : i1 loc(#loc138)
    %2198 = arith.ori %2045, %2095 : i1
    acir.path %2040 {branch = 49 : i64, kind = "branch", txn = 6 : i64} loc(#loc133)
    acir.path %2040 {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2199 = arith.ori %2045, %2198 : i1 loc(#loc133)
    acir.path %2040 {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate", "top.rob.slots.0:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc133)
    %2200 = arith.ori %2045, %2199 : i1 loc(#loc133)
    %value_112, %ready_113 = acir.read(%2040) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate", "top.rob.slots.0:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc134)
    %2201 = arith.andi %2200, %ready_113 : i1 loc(#loc134)
    %2202 = arith.ori %2045, %2201 : i1 loc(#loc134)
    %2203 = arith.andi %2197, %2202 : i1 loc(#loc134)
    %2204 = acir.extract %value_112 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
    %2205 = acir.unary %2204 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
    %2206 = arith.ori %2045, %2200 : i1
    %2207 = arith.andi %2203, %2206 : i1
    %2208 = arith.andi %2040, %2201 : i1
    %2209 = arith.andi %2199, %2208 : i1 loc(#loc136)
    %2210 = arith.andi %2040, %2209 : i1 loc(#loc136)
    %2211 = arith.andi %2210, %2205 : i1 loc(#loc136)
    %2212 = arith.andi %2198, %2211 : i1 loc(#loc136)
    %2213 = arith.ori %2045, %2212 : i1 loc(#loc136)
    %2214 = arith.andi %2207, %2213 : i1 loc(#loc136)
    %2215 = arith.andi %2212, %2092 : i1 loc(#loc136)
    %2216 = arith.andi %2040, %2215 : i1 loc(#loc136)
    %2217 = arith.andi %2212, %2094 : i1 loc(#loc136)
    %2218 = arith.andi %2040, %2217 : i1 loc(#loc136)
    %2219 = arith.andi %2216, %2218 : i1 loc(#loc137)
    %2220 = arith.andi %2213, %2219 : i1 loc(#loc138)
    %2221 = arith.ori %2045, %2220 : i1 loc(#loc138)
    %2222 = arith.andi %2214, %2221 : i1 loc(#loc138)
    %2223 = arith.addi %value_104, %c1_i32 {acir.id = "v24", acir.source_type = "u32"} : i32 loc(#loc139)
    %2224 = arith.andi %2222, %2027 : i1 loc(#loc140)
    %2225 = arith.andi %2224, %2035 : i1
    %2226 = arith.andi %2036, %2020 : i1
    %2227 = acir.extract %value_68 {acir.id = "v12", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc141)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc142)
    %2228 = arith.andi %1240, %2226 : i1 loc(#loc142)
    %2229 = arith.andi %1236, %2228 : i1 loc(#loc142)
    %2230 = arith.cmpi ult, %2227, %c8_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc143)
    %2231 = arith.andi %2225, %1258 : i1 loc(#loc144)
    %2232 = arith.cmpi ne, %2227, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc145)
    %2233 = arith.andi %2231, %1258 : i1
    %2234 = arith.andi %1236, %2232 : i1
    %2235 = arith.xori %2234, %true : i1 loc(#loc146)
    %2236 = arith.ori %2235, %1257 : i1
    acir.path %2234 {branch = 50 : i64, kind = "branch", txn = 6 : i64} loc(#loc146)
    %value_114, %ready_115 = acir.read(%2234) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc146)
    %2237 = arith.andi %2236, %ready_115 : i1 loc(#loc146)
    %2238 = arith.ori %2235, %2237 : i1 loc(#loc146)
    %2239 = arith.andi %2233, %2238 : i1 loc(#loc146)
    %2240 = arith.andi %ready_115, %2229 : i1 loc(#loc146)
    %2241 = arith.andi %2240, %1254 : i1 loc(#loc146)
    %2242 = arith.extui %2227 : i32 to i64 loc(#loc146)
    %2243 = arith.cmpi ult, %2242, %c8_i64 : i64 loc(#loc146)
    %2244 = arith.andi %2241, %2243 : i1 loc(#loc146)
    %2245 = arith.select %2243, %2242, %c0_i64 : i64 loc(#loc146)
    %2246 = acir.update %value_114, %value_104[%2245] {acir.id = "v15", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i64) -> !acir.array<i32, [8]> loc(#loc146)
    %2247 = arith.andi %2236, %2244 : i1 loc(#loc146)
    %2248 = arith.ori %2235, %2247 : i1 loc(#loc146)
    %2249 = arith.andi %2239, %2248 : i1 loc(#loc146)
    %2250 = acir.extract %value_68 {acir.id = "v15", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc147)
    %2251 = acir.extract %value_68 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc148)
    %2252 = arith.andi %ready_69, %2226 : i1 loc(#loc149)
    %2253 = arith.andi %2252, %1632 : i1 loc(#loc149)
    %2254 = arith.andi %2253, %2004 : i1 loc(#loc149)
    %2255 = acir.aggregate(%true, %2250, %value_104, %2251, %1625, %1998) {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc149)
    acir.path %1236 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc150)
    %value_116, %ready_117 = acir.read(%1236) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    %2256 = arith.andi %1253, %ready_117 : i1 loc(#loc2)
    %2257 = arith.ori %1237, %2256 : i1 loc(#loc2)
    %2258 = arith.andi %2249, %2257 : i1 loc(#loc2)
    %2259 = arith.andi %2258, %1274 : i1
    %2260 = arith.andi %1236, %2256 : i1
    %2261 = acir.extract %value_116 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc150)
    %2262 = acir.unary %2261 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc151)
    %2263 = arith.andi %1240, %2260 : i1
    %2264 = arith.ori %1237, %2263 : i1
    %2265 = arith.andi %2259, %2264 : i1
    %2266 = arith.andi %1236, %2262 : i1
    %2267 = arith.xori %2262, %true : i1
    %2268 = arith.andi %1236, %2267 : i1
    %2269 = arith.xori %2268, %true : i1 loc(#loc152)
    %2270 = arith.ori %2269, %2263 : i1
    acir.path %2268 {branch = 51 : i64, kind = "branch", txn = 6 : i64} loc(#loc152)
    acir.path %2268 {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc152)
    %2271 = arith.ori %2269, %2270 : i1 loc(#loc152)
    acir.path %2268 {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc152)
    %2272 = arith.ori %2269, %2271 : i1 loc(#loc152)
    %value_118, %ready_119 = acir.read(%2268) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc153)
    %2273 = arith.andi %2272, %ready_119 : i1 loc(#loc153)
    %2274 = arith.ori %2269, %2273 : i1 loc(#loc153)
    %2275 = arith.andi %2265, %2274 : i1 loc(#loc153)
    %2276 = acir.extract %value_118 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc153)
    %2277 = acir.unary %2276 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc154)
    %2278 = arith.ori %2269, %2272 : i1
    %2279 = arith.andi %2275, %2278 : i1
    %2280 = arith.andi %2268, %2273 : i1
    %2281 = arith.andi %2271, %2280 : i1 loc(#loc155)
    %2282 = arith.andi %2268, %2281 : i1 loc(#loc155)
    %2283 = arith.andi %2282, %2277 : i1 loc(#loc155)
    %2284 = arith.andi %2270, %2283 : i1 loc(#loc155)
    %2285 = arith.ori %2269, %2284 : i1 loc(#loc155)
    %2286 = arith.andi %2279, %2285 : i1 loc(#loc155)
    %2287 = arith.andi %2284, %2254 : i1 loc(#loc155)
    %2288 = arith.andi %2268, %2287 : i1 loc(#loc155)
    %2289 = arith.andi %2285, %2288 : i1 loc(#loc156)
    %2290 = arith.ori %2269, %2289 : i1 loc(#loc156)
    %2291 = arith.andi %2286, %2290 : i1 loc(#loc156)
    %2292 = arith.xori %2266, %true : i1 loc(#loc157)
    %2293 = arith.ori %2292, %2263 : i1
    acir.path %2266 {branch = 52 : i64, kind = "branch", txn = 6 : i64} loc(#loc157)
    acir.path %2266 {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc157)
    %2294 = arith.ori %2292, %2293 : i1 loc(#loc157)
    acir.path %2266 {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc157)
    %2295 = arith.ori %2292, %2294 : i1 loc(#loc157)
    %value_120, %ready_121 = acir.read(%2266) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc153)
    %2296 = arith.andi %2295, %ready_121 : i1 loc(#loc153)
    %2297 = arith.ori %2292, %2296 : i1 loc(#loc153)
    %2298 = arith.andi %2291, %2297 : i1 loc(#loc153)
    %2299 = acir.extract %value_120 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc153)
    %2300 = acir.unary %2299 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc154)
    %2301 = arith.ori %2292, %2295 : i1
    %2302 = arith.andi %2298, %2301 : i1
    %2303 = arith.andi %2266, %2296 : i1
    %2304 = arith.andi %2294, %2303 : i1 loc(#loc155)
    %2305 = arith.andi %2266, %2304 : i1 loc(#loc155)
    %2306 = arith.andi %2305, %2300 : i1 loc(#loc155)
    %2307 = arith.andi %2293, %2306 : i1 loc(#loc155)
    %2308 = arith.ori %2292, %2307 : i1 loc(#loc155)
    %2309 = arith.andi %2302, %2308 : i1 loc(#loc155)
    %2310 = arith.andi %2307, %2254 : i1 loc(#loc155)
    %2311 = arith.andi %2266, %2310 : i1 loc(#loc155)
    %2312 = arith.andi %2308, %2311 : i1 loc(#loc156)
    %2313 = arith.ori %2292, %2312 : i1 loc(#loc156)
    %2314 = arith.andi %2309, %2313 : i1 loc(#loc156)
    %2315 = acir.fire "top.dispatch.dispatch#6" %2314 {successors = [], txn = 6 : i64}
    %2316 = arith.andi %2315, %1236 : i1
    acir.effect %2316() {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "pop", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> () loc(#loc100)
    acir.effect %2316(%1256) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc104)
    %2317 = arith.andi %2315, %1429 : i1
    acir.effect %2317(%0) {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2318 = arith.andi %2315, %1421 : i1
    acir.effect %2318(%0) {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2319 = arith.andi %2315, %1413 : i1
    acir.effect %2319(%0) {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2320 = arith.andi %2315, %1352 : i1
    acir.effect %2320(%0) {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2321 = arith.andi %2315, %1330 : i1
    acir.effect %2321(%1513) {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2321(%0) {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2316(%1627) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2316(%1634) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc104)
    %2322 = arith.andi %2315, %1802 : i1
    acir.effect %2322(%0) {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2323 = arith.andi %2315, %1794 : i1
    acir.effect %2323(%0) {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2324 = arith.andi %2315, %1786 : i1
    acir.effect %2324(%0) {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2325 = arith.andi %2315, %1726 : i1
    acir.effect %2325(%0) {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2326 = arith.andi %2315, %1704 : i1
    acir.effect %2326(%1886) {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2326(%0) {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2316(%1999) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2316(%0) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2327 = arith.andi %2315, %2118 : i1
    acir.effect %2327(%0) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate", "top.rob.slots.3:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2327(%2142) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc138)
    %2328 = arith.andi %2315, %2110 : i1
    acir.effect %2328(%0) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate", "top.rob.slots.2:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2328(%2142) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc138)
    %2329 = arith.andi %2315, %2102 : i1
    acir.effect %2329(%0) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate", "top.rob.slots.1:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2329(%2142) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc138)
    %2330 = arith.andi %2315, %2040 : i1
    acir.effect %2330(%0) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate", "top.rob.slots.0:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2330(%2142) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc138)
    acir.effect %2316(%2223) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "write", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1, i32) -> () loc(#loc140)
    acir.effect %2316(%0) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2316(%2230) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc144)
    %2331 = arith.andi %2315, %2234 : i1
    acir.effect %2331(%2246) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "write", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1, !acir.array<i32, [8]>) -> () loc(#loc146)
    acir.effect %2316(%0) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2332 = arith.andi %2315, %2268 : i1
    acir.effect %2332(%0) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2332(%2255) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc156)
    %2333 = arith.andi %2315, %2266 : i1
    acir.effect %2333(%0) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2333(%2255) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc156)
    acir.path %true {branch = 53 : i64, kind = "branch", txn = 7 : i64} loc(#loc158)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc158)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc159)
    %value_122, %ready_123 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc160)
    %2334 = arith.subi %value_122, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc161)
    %2335 = arith.andi %2334, %c3_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc162)
    %2336 = arith.cmpi eq, %2335, %c0_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc163)
    %2337 = arith.cmpi eq, %2335, %c1_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc163)
    %2338 = arith.cmpi eq, %2335, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc163)
    %2339 = arith.cmpi eq, %2335, %c3_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc163)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2340 = arith.cmpi ne, %2335, %c0_i32 : i32 loc(#loc163)
    %2341 = arith.cmpi eq, %2335, %c0_i32 : i32 loc(#loc163)
    %2342 = arith.ori %2341, %ready_123 : i1
    %2343 = arith.ori %2340, %ready_123 : i1
    %2344 = arith.andi %2342, %ready_123 : i1
    %2345 = arith.andi %2340, %2344 : i1
    %2346 = arith.andi %2336, %2343 : i1
    %2347 = arith.ori %2345, %2346 : i1
    %2348 = arith.select %2340, %2337, %true : i1
    %2349 = arith.andi %ready_123, %2347 : i1
    %2350 = arith.xori %2348, %true : i1
    %2351 = arith.ori %2348, %2347 : i1
    %2352 = arith.ori %2350, %2347 : i1
    %2353 = arith.andi %2351, %ready_123 : i1
    %2354 = arith.andi %2350, %2353 : i1
    %2355 = arith.andi %2348, %2352 : i1
    %2356 = arith.ori %2354, %2355 : i1
    %2357 = arith.select %2348, %true, %2338 : i1
    %2358 = arith.andi %2349, %2356 : i1
    %2359 = arith.xori %2357, %true : i1
    %2360 = arith.ori %2357, %2356 : i1
    %2361 = arith.ori %2359, %2356 : i1
    %2362 = arith.andi %2360, %ready_123 : i1
    %2363 = arith.andi %2359, %2362 : i1
    %2364 = arith.andi %2357, %2361 : i1
    %2365 = arith.ori %2363, %2364 : i1
    %2366 = arith.select %2357, %true, %2339 : i1
    %2367 = arith.andi %2365, %2366 : i1 loc(#loc163)
    %2368 = arith.andi %2358, %2367 : i1 loc(#loc163)
    %2369 = arith.andi %2367, %ready_123 : i1 loc(#loc163)
    %2370 = arith.andi %2368, %2369 : i1
    %2371 = arith.ori %2341, %2369 : i1
    %2372 = arith.andi %2371, %2369 : i1
    %2373 = arith.ori %2341, %2372 : i1
    %2374 = arith.andi %2370, %2373 : i1
    %2375 = arith.andi %2340, %2337 : i1
    %2376 = arith.cmpi ne, %2335, %c1_i32 : i32 loc(#loc163)
    %2377 = arith.andi %2340, %2376 : i1
    %2378 = arith.xori %2377, %true : i1
    %2379 = arith.ori %2378, %2372 : i1
    %2380 = arith.andi %2379, %2369 : i1
    %2381 = arith.ori %2378, %2380 : i1
    %2382 = arith.andi %2374, %2381 : i1
    %2383 = arith.andi %2377, %2338 : i1
    %2384 = arith.cmpi ne, %2335, %c2_i32 : i32 loc(#loc163)
    %2385 = arith.andi %2377, %2384 : i1
    %2386 = arith.xori %2385, %true : i1
    %2387 = arith.ori %2386, %2380 : i1
    %2388 = arith.andi %2387, %2369 : i1
    %2389 = arith.ori %2386, %2388 : i1
    %2390 = arith.andi %2382, %2389 : i1
    %2391 = arith.andi %2385, %2339 : i1
    %2392 = arith.cmpi ne, %2335, %c3_i32 : i32 loc(#loc163)
    %2393 = arith.andi %2385, %2392 : i1
    %2394 = arith.xori %2393, %true : i1
    %2395 = arith.ori %2394, %2388 : i1
    %2396 = arith.xori %2391, %true : i1 loc(#loc163)
    %2397 = arith.ori %2396, %2388 : i1
    acir.path %2391 {branch = 54 : i64, kind = "branch", txn = 7 : i64} loc(#loc163)
    acir.path %2391 {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2398 = arith.ori %2396, %2397 : i1 loc(#loc163)
    %2399 = arith.andi %2397, %2369 : i1 loc(#loc163)
    %2400 = arith.andi %2391, %2399 : i1 loc(#loc163)
    acir.path %2391 {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire", "top.rob.slots.3:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2401 = arith.ori %2396, %2398 : i1 loc(#loc163)
    %2402 = arith.andi %2398, %2400 : i1 loc(#loc163)
    %2403 = arith.andi %2391, %2402 : i1 loc(#loc163)
    %value_124, %ready_125 = acir.read(%2391) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire", "top.rob.slots.3:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc164)
    %2404 = arith.andi %2401, %ready_125 : i1 loc(#loc164)
    %2405 = arith.ori %2396, %2404 : i1 loc(#loc164)
    %2406 = arith.andi %2390, %2405 : i1 loc(#loc164)
    %2407 = acir.extract %value_124 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc165)
    %2408 = arith.andi %2406, %2405 : i1
    %2409 = arith.andi %2391, %2407 : i1
    %2410 = arith.xori %2407, %true : i1
    %2411 = arith.andi %2391, %2410 : i1
    %2412 = arith.xori %2411, %true : i1
    %2413 = arith.ori %2412, %2404 : i1
    %2414 = arith.xori %2409, %true : i1
    %2415 = arith.ori %2414, %2404 : i1
    %2416 = acir.extract %value_124 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc166)
    %2417 = arith.andi %2411, %2413 : i1
    %2418 = arith.andi %2415, %ready_125 : i1
    %2419 = arith.andi %2409, %2418 : i1
    %2420 = arith.ori %2417, %2419 : i1
    %2421 = arith.select %2411, %false, %2416 : i1
    %2422 = arith.ori %2396, %2401 : i1
    %2423 = arith.andi %2422, %2420 : i1
    %2424 = arith.ori %2396, %2423 : i1
    %2425 = arith.andi %2408, %2424 : i1
    %2426 = arith.andi %2391, %2421 : i1
    %2427 = arith.xori %2421, %true : i1
    %2428 = arith.andi %2391, %2427 : i1
    %2429 = arith.xori %2428, %true : i1
    %2430 = arith.ori %2429, %2423 : i1
    %2431 = arith.xori %2426, %true : i1
    %2432 = arith.ori %2431, %2423 : i1
    %2433 = acir.extract %value_124 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc167)
    %2434 = arith.andi %ready_125, %2403 : i1 loc(#loc167)
    %2435 = arith.cmpi eq, %2433, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc167)
    %2436 = arith.andi %2428, %2430 : i1
    %2437 = arith.andi %2432, %2434 : i1
    %2438 = arith.andi %2426, %2437 : i1
    %2439 = arith.ori %2436, %2438 : i1
    %2440 = arith.select %2428, %false, %2435 : i1
    %2441 = arith.ori %2396, %2422 : i1
    %2442 = arith.ori %2396, %2441 : i1
    %2443 = arith.andi %2425, %2442 : i1
    %2444 = arith.andi %2441, %2439 : i1
    %2445 = arith.andi %2391, %2444 : i1
    %2446 = arith.andi %2398, %2445 : i1 loc(#loc168)
    %2447 = arith.andi %2391, %2446 : i1 loc(#loc168)
    %2448 = arith.andi %2447, %2440 : i1 loc(#loc168)
    %2449 = arith.andi %2397, %2448 : i1 loc(#loc168)
    %2450 = arith.ori %2396, %2449 : i1 loc(#loc168)
    %2451 = arith.andi %2443, %2450 : i1 loc(#loc168)
    %value_126, %ready_127 = acir.read(%2391) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc169)
    %2452 = arith.andi %2450, %ready_127 : i1 loc(#loc169)
    %2453 = arith.ori %2396, %2452 : i1 loc(#loc169)
    %2454 = arith.andi %2451, %2453 : i1 loc(#loc169)
    %2455 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc170)
    %2456 = arith.ori %2396, %2450 : i1 loc(#loc171)
    %2457 = arith.andi %2454, %2456 : i1 loc(#loc171)
    %2458 = arith.andi %2457, %2456 : i1
    %2459 = arith.andi %2391, %2452 : i1
    %2460 = arith.andi %2459, %2448 : i1
    %2461 = arith.andi %2393, %2395 : i1
    %2462 = arith.andi %2391, %2397 : i1
    %2463 = arith.ori %2461, %2462 : i1
    %2464 = arith.andi %2397, %2460 : i1
    %2465 = arith.andi %2391, %2464 : i1
    %2466 = arith.ori %2461, %2465 : i1
    %2467 = arith.select %2393, %1405, %value_126 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2468 = arith.ori %2386, %2387 : i1
    %2469 = arith.xori %2383, %true : i1 loc(#loc163)
    %2470 = arith.ori %2469, %2380 : i1
    acir.path %2383 {branch = 55 : i64, kind = "branch", txn = 7 : i64} loc(#loc163)
    acir.path %2383 {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2471 = arith.ori %2469, %2470 : i1 loc(#loc163)
    %2472 = arith.andi %2470, %2369 : i1 loc(#loc163)
    %2473 = arith.andi %2383, %2472 : i1 loc(#loc163)
    acir.path %2383 {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire", "top.rob.slots.2:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2474 = arith.ori %2469, %2471 : i1 loc(#loc163)
    %2475 = arith.andi %2471, %2473 : i1 loc(#loc163)
    %2476 = arith.andi %2383, %2475 : i1 loc(#loc163)
    %value_128, %ready_129 = acir.read(%2383) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire", "top.rob.slots.2:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc164)
    %2477 = arith.andi %2474, %ready_129 : i1 loc(#loc164)
    %2478 = arith.ori %2469, %2477 : i1 loc(#loc164)
    %2479 = arith.andi %2458, %2478 : i1 loc(#loc164)
    %2480 = acir.extract %value_128 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc165)
    %2481 = arith.andi %2479, %2478 : i1
    %2482 = arith.andi %2383, %2480 : i1
    %2483 = arith.xori %2480, %true : i1
    %2484 = arith.andi %2383, %2483 : i1
    %2485 = arith.xori %2484, %true : i1
    %2486 = arith.ori %2485, %2477 : i1
    %2487 = arith.xori %2482, %true : i1
    %2488 = arith.ori %2487, %2477 : i1
    %2489 = acir.extract %value_128 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc166)
    %2490 = arith.andi %2484, %2486 : i1
    %2491 = arith.andi %2488, %ready_129 : i1
    %2492 = arith.andi %2482, %2491 : i1
    %2493 = arith.ori %2490, %2492 : i1
    %2494 = arith.select %2484, %false, %2489 : i1
    %2495 = arith.ori %2469, %2474 : i1
    %2496 = arith.andi %2495, %2493 : i1
    %2497 = arith.ori %2469, %2496 : i1
    %2498 = arith.andi %2481, %2497 : i1
    %2499 = arith.andi %2383, %2494 : i1
    %2500 = arith.xori %2494, %true : i1
    %2501 = arith.andi %2383, %2500 : i1
    %2502 = arith.xori %2501, %true : i1
    %2503 = arith.ori %2502, %2496 : i1
    %2504 = arith.xori %2499, %true : i1
    %2505 = arith.ori %2504, %2496 : i1
    %2506 = acir.extract %value_128 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc167)
    %2507 = arith.andi %ready_129, %2476 : i1 loc(#loc167)
    %2508 = arith.cmpi eq, %2506, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc167)
    %2509 = arith.andi %2501, %2503 : i1
    %2510 = arith.andi %2505, %2507 : i1
    %2511 = arith.andi %2499, %2510 : i1
    %2512 = arith.ori %2509, %2511 : i1
    %2513 = arith.select %2501, %false, %2508 : i1
    %2514 = arith.ori %2469, %2495 : i1
    %2515 = arith.ori %2469, %2514 : i1
    %2516 = arith.andi %2498, %2515 : i1
    %2517 = arith.andi %2514, %2512 : i1
    %2518 = arith.andi %2383, %2517 : i1
    %2519 = arith.andi %2471, %2518 : i1 loc(#loc168)
    %2520 = arith.andi %2383, %2519 : i1 loc(#loc168)
    %2521 = arith.andi %2520, %2513 : i1 loc(#loc168)
    %2522 = arith.andi %2470, %2521 : i1 loc(#loc168)
    %2523 = arith.ori %2469, %2522 : i1 loc(#loc168)
    %2524 = arith.andi %2516, %2523 : i1 loc(#loc168)
    %value_130, %ready_131 = acir.read(%2383) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc169)
    %2525 = arith.andi %2523, %ready_131 : i1 loc(#loc169)
    %2526 = arith.ori %2469, %2525 : i1 loc(#loc169)
    %2527 = arith.andi %2524, %2526 : i1 loc(#loc169)
    %2528 = arith.ori %2469, %2523 : i1 loc(#loc171)
    %2529 = arith.andi %2527, %2528 : i1 loc(#loc171)
    %2530 = arith.andi %2529, %2528 : i1
    %2531 = arith.andi %2383, %2525 : i1
    %2532 = arith.andi %2531, %2521 : i1
    %2533 = arith.andi %2468, %2463 : i1
    %2534 = arith.andi %2385, %2533 : i1
    %2535 = arith.andi %2383, %2470 : i1
    %2536 = arith.ori %2534, %2535 : i1
    %2537 = arith.select %2385, %2393, %false : i1
    %2538 = arith.andi %2468, %2466 : i1
    %2539 = arith.andi %2385, %2538 : i1
    %2540 = arith.andi %2470, %2532 : i1
    %2541 = arith.andi %2383, %2540 : i1
    %2542 = arith.ori %2539, %2541 : i1
    %2543 = arith.select %2385, %2467, %value_130 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2544 = arith.ori %2378, %2379 : i1
    %2545 = arith.xori %2375, %true : i1 loc(#loc163)
    %2546 = arith.ori %2545, %2372 : i1
    acir.path %2375 {branch = 56 : i64, kind = "branch", txn = 7 : i64} loc(#loc163)
    acir.path %2375 {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2547 = arith.ori %2545, %2546 : i1 loc(#loc163)
    %2548 = arith.andi %2546, %2369 : i1 loc(#loc163)
    %2549 = arith.andi %2375, %2548 : i1 loc(#loc163)
    acir.path %2375 {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire", "top.rob.slots.1:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2550 = arith.ori %2545, %2547 : i1 loc(#loc163)
    %2551 = arith.andi %2547, %2549 : i1 loc(#loc163)
    %2552 = arith.andi %2375, %2551 : i1 loc(#loc163)
    %value_132, %ready_133 = acir.read(%2375) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire", "top.rob.slots.1:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc164)
    %2553 = arith.andi %2550, %ready_133 : i1 loc(#loc164)
    %2554 = arith.ori %2545, %2553 : i1 loc(#loc164)
    %2555 = arith.andi %2530, %2554 : i1 loc(#loc164)
    %2556 = acir.extract %value_132 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc165)
    %2557 = arith.andi %2555, %2554 : i1
    %2558 = arith.andi %2375, %2556 : i1
    %2559 = arith.xori %2556, %true : i1
    %2560 = arith.andi %2375, %2559 : i1
    %2561 = arith.xori %2560, %true : i1
    %2562 = arith.ori %2561, %2553 : i1
    %2563 = arith.xori %2558, %true : i1
    %2564 = arith.ori %2563, %2553 : i1
    %2565 = acir.extract %value_132 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc166)
    %2566 = arith.andi %2560, %2562 : i1
    %2567 = arith.andi %2564, %ready_133 : i1
    %2568 = arith.andi %2558, %2567 : i1
    %2569 = arith.ori %2566, %2568 : i1
    %2570 = arith.select %2560, %false, %2565 : i1
    %2571 = arith.ori %2545, %2550 : i1
    %2572 = arith.andi %2571, %2569 : i1
    %2573 = arith.ori %2545, %2572 : i1
    %2574 = arith.andi %2557, %2573 : i1
    %2575 = arith.andi %2375, %2570 : i1
    %2576 = arith.xori %2570, %true : i1
    %2577 = arith.andi %2375, %2576 : i1
    %2578 = arith.xori %2577, %true : i1
    %2579 = arith.ori %2578, %2572 : i1
    %2580 = arith.xori %2575, %true : i1
    %2581 = arith.ori %2580, %2572 : i1
    %2582 = acir.extract %value_132 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc167)
    %2583 = arith.andi %ready_133, %2552 : i1 loc(#loc167)
    %2584 = arith.cmpi eq, %2582, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc167)
    %2585 = arith.andi %2577, %2579 : i1
    %2586 = arith.andi %2581, %2583 : i1
    %2587 = arith.andi %2575, %2586 : i1
    %2588 = arith.ori %2585, %2587 : i1
    %2589 = arith.select %2577, %false, %2584 : i1
    %2590 = arith.ori %2545, %2571 : i1
    %2591 = arith.ori %2545, %2590 : i1
    %2592 = arith.andi %2574, %2591 : i1
    %2593 = arith.andi %2590, %2588 : i1
    %2594 = arith.andi %2375, %2593 : i1
    %2595 = arith.andi %2547, %2594 : i1 loc(#loc168)
    %2596 = arith.andi %2375, %2595 : i1 loc(#loc168)
    %2597 = arith.andi %2596, %2589 : i1 loc(#loc168)
    %2598 = arith.andi %2546, %2597 : i1 loc(#loc168)
    %2599 = arith.ori %2545, %2598 : i1 loc(#loc168)
    %2600 = arith.andi %2592, %2599 : i1 loc(#loc168)
    %value_134, %ready_135 = acir.read(%2375) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc169)
    %2601 = arith.andi %2599, %ready_135 : i1 loc(#loc169)
    %2602 = arith.ori %2545, %2601 : i1 loc(#loc169)
    %2603 = arith.andi %2600, %2602 : i1 loc(#loc169)
    %2604 = arith.ori %2545, %2599 : i1 loc(#loc171)
    %2605 = arith.andi %2603, %2604 : i1 loc(#loc171)
    %2606 = arith.andi %2605, %2604 : i1
    %2607 = arith.andi %2375, %2601 : i1
    %2608 = arith.andi %2607, %2597 : i1
    %2609 = arith.andi %2544, %2536 : i1
    %2610 = arith.andi %2377, %2609 : i1
    %2611 = arith.andi %2375, %2546 : i1
    %2612 = arith.ori %2610, %2611 : i1
    %2613 = arith.select %2377, %2537, %false : i1
    %2614 = arith.andi %2544, %2542 : i1
    %2615 = arith.andi %2377, %2614 : i1
    %2616 = arith.andi %2546, %2608 : i1
    %2617 = arith.andi %2375, %2616 : i1
    %2618 = arith.ori %2615, %2617 : i1
    %2619 = arith.select %2377, %2543, %value_134 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2620 = arith.ori %2341, %2371 : i1
    %2621 = arith.ori %2340, %2369 : i1
    acir.path %2336 {branch = 57 : i64, kind = "branch", txn = 7 : i64} loc(#loc163)
    acir.path %2336 {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2622 = arith.ori %2340, %2621 : i1 loc(#loc163)
    %2623 = arith.andi %2621, %2369 : i1 loc(#loc163)
    %2624 = arith.andi %2336, %2623 : i1 loc(#loc163)
    acir.path %2336 {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire", "top.rob.slots.0:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc163)
    %2625 = arith.ori %2340, %2622 : i1 loc(#loc163)
    %2626 = arith.andi %2622, %2624 : i1 loc(#loc163)
    %2627 = arith.andi %2336, %2626 : i1 loc(#loc163)
    %value_136, %ready_137 = acir.read(%2336) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire", "top.rob.slots.0:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc164)
    %2628 = arith.andi %2625, %ready_137 : i1 loc(#loc164)
    %2629 = arith.ori %2340, %2628 : i1 loc(#loc164)
    %2630 = arith.andi %2606, %2629 : i1 loc(#loc164)
    %2631 = acir.extract %value_136 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc165)
    %2632 = arith.andi %2630, %2629 : i1
    %2633 = arith.andi %2336, %2631 : i1
    %2634 = arith.xori %2631, %true : i1
    %2635 = arith.andi %2336, %2634 : i1
    %2636 = arith.xori %2635, %true : i1
    %2637 = arith.ori %2636, %2628 : i1
    %2638 = arith.xori %2633, %true : i1
    %2639 = arith.ori %2638, %2628 : i1
    %2640 = acir.extract %value_136 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc166)
    %2641 = arith.andi %2635, %2637 : i1
    %2642 = arith.andi %2639, %ready_137 : i1
    %2643 = arith.andi %2633, %2642 : i1
    %2644 = arith.ori %2641, %2643 : i1
    %2645 = arith.select %2635, %false, %2640 : i1
    %2646 = arith.ori %2340, %2625 : i1
    %2647 = arith.andi %2646, %2644 : i1
    %2648 = arith.ori %2340, %2647 : i1
    %2649 = arith.andi %2632, %2648 : i1
    %2650 = arith.andi %2336, %2645 : i1
    %2651 = arith.xori %2645, %true : i1
    %2652 = arith.andi %2336, %2651 : i1
    %2653 = arith.xori %2652, %true : i1
    %2654 = arith.ori %2653, %2647 : i1
    %2655 = arith.xori %2650, %true : i1
    %2656 = arith.ori %2655, %2647 : i1
    %2657 = acir.extract %value_136 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc167)
    %2658 = arith.andi %ready_137, %2627 : i1 loc(#loc167)
    %2659 = arith.cmpi eq, %2657, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc167)
    %2660 = arith.andi %2652, %2654 : i1
    %2661 = arith.andi %2656, %2658 : i1
    %2662 = arith.andi %2650, %2661 : i1
    %2663 = arith.ori %2660, %2662 : i1
    %2664 = arith.select %2652, %false, %2659 : i1
    %2665 = arith.ori %2340, %2646 : i1
    %2666 = arith.ori %2340, %2665 : i1
    %2667 = arith.andi %2649, %2666 : i1
    %2668 = arith.andi %2665, %2663 : i1
    %2669 = arith.andi %2336, %2668 : i1
    %2670 = arith.andi %2622, %2669 : i1 loc(#loc168)
    %2671 = arith.andi %2336, %2670 : i1 loc(#loc168)
    %2672 = arith.andi %2671, %2664 : i1 loc(#loc168)
    %2673 = arith.andi %2621, %2672 : i1 loc(#loc168)
    %2674 = arith.ori %2340, %2673 : i1 loc(#loc168)
    %2675 = arith.andi %2667, %2674 : i1 loc(#loc168)
    %value_138, %ready_139 = acir.read(%2336) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc169)
    %2676 = arith.andi %2674, %ready_139 : i1 loc(#loc169)
    %2677 = arith.ori %2340, %2676 : i1 loc(#loc169)
    %2678 = arith.andi %2675, %2677 : i1 loc(#loc169)
    %2679 = arith.ori %2340, %2674 : i1 loc(#loc171)
    %2680 = arith.andi %2678, %2679 : i1 loc(#loc171)
    %2681 = arith.andi %2680, %2679 : i1
    %2682 = arith.andi %2336, %2676 : i1
    %2683 = arith.andi %2682, %2672 : i1
    %2684 = arith.andi %2620, %2612 : i1
    %2685 = arith.andi %2340, %2684 : i1
    %2686 = arith.andi %2336, %2621 : i1
    %2687 = arith.ori %2685, %2686 : i1
    %2688 = arith.select %2340, %2613, %false : i1
    %2689 = arith.andi %2620, %2618 : i1
    %2690 = arith.andi %2340, %2689 : i1
    %2691 = arith.andi %2621, %2683 : i1
    %2692 = arith.andi %2336, %2691 : i1
    %2693 = arith.ori %2690, %2692 : i1
    %2694 = arith.select %2340, %2619, %value_138 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2695 = acir.unary %2688 "not" {acir.source_type = "bool"} : i1 -> i1
    %2696 = arith.andi %2367, %2687 : i1
    %2697 = arith.andi %2681, %2696 : i1
    %2698 = arith.andi %2367, %2693 : i1
    %2699 = arith.addi %value_122, %c1_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc172)
    %2700 = arith.andi %2697, %ready_123 : i1 loc(#loc173)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc174)
    %2701 = acir.extract %2694 {acir.id = "v5", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc175)
    %2702 = arith.cmpi ne, %2701, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc175)
    %2703 = arith.andi %2700, %2698 : i1
    %2704 = arith.cmpi eq, %2701, %c0_i32 : i32 loc(#loc175)
    %2705 = arith.ori %2704, %2698 : i1
    %2706 = acir.extract %2694 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc176)
    acir.path %2702 {branch = 58 : i64, kind = "branch", txn = 7 : i64} loc(#loc177)
    %value_140, %ready_141 = acir.read(%2702) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc177)
    %2707 = arith.andi %2705, %ready_141 : i1 loc(#loc177)
    %2708 = arith.ori %2704, %2707 : i1 loc(#loc177)
    %2709 = arith.andi %2703, %2708 : i1 loc(#loc177)
    %2710 = acir.extract %2694 {acir.id = "v13", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc178)
    %2711 = arith.andi %ready_141, %2698 : i1 loc(#loc177)
    %2712 = arith.extui %2710 : i32 to i64 loc(#loc177)
    %2713 = arith.cmpi ult, %2712, %c8_i64 : i64 loc(#loc177)
    %2714 = arith.andi %2711, %2713 : i1 loc(#loc177)
    %2715 = arith.select %2713, %2712, %c0_i64 : i64 loc(#loc177)
    %2716 = acir.update %value_140, %2706[%2715] {acir.id = "v14", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i64) -> !acir.array<i32, [8]> loc(#loc177)
    %2717 = arith.andi %2705, %2714 : i1 loc(#loc177)
    %2718 = arith.ori %2704, %2717 : i1 loc(#loc177)
    %2719 = arith.andi %2709, %2718 : i1 loc(#loc177)
    %2720 = acir.extract %2694 {acir.id = "v17", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc179)
    %value_142, %ready_143 = acir.read(%2702) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc180)
    %2721 = arith.andi %2705, %ready_143 : i1 loc(#loc180)
    %2722 = arith.ori %2704, %2721 : i1 loc(#loc180)
    %2723 = arith.andi %2719, %2722 : i1 loc(#loc180)
    %2724 = acir.extract %2694 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc181)
    %2725 = arith.andi %ready_143, %2698 : i1 loc(#loc180)
    %2726 = arith.extui %2724 : i32 to i64 loc(#loc180)
    %2727 = arith.cmpi ult, %2726, %c8_i64 : i64 loc(#loc180)
    %2728 = arith.andi %2725, %2727 : i1 loc(#loc180)
    %2729 = arith.select %2727, %2726, %c0_i64 : i64 loc(#loc180)
    %2730 = acir.update %value_142, %2720[%2729] {acir.id = "v22", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i64) -> !acir.array<i32, [8]> loc(#loc180)
    %2731 = arith.andi %2705, %2728 : i1 loc(#loc180)
    %2732 = arith.ori %2704, %2731 : i1 loc(#loc180)
    %2733 = arith.andi %2723, %2732 : i1 loc(#loc180)
    %value_144, %ready_145 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit"], method = "read", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc182)
    %2734 = arith.andi %2733, %ready_145 : i1 loc(#loc182)
    %2735 = arith.addi %value_144, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc182)
    %2736 = acir.extract %2694 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc183)
    %2737 = arith.andi %2734, %2698 : i1 loc(#loc184)
    %2738 = acir.fire "top.commit.commit#7" %2737 {successors = [], txn = 7 : i64}
    %2739 = arith.andi %2738, %2391 : i1
    acir.effect %2739(%0) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire", "top.rob.slots.3:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2739(%2455) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc171)
    acir.effect %2739(%0) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2740 = arith.andi %2738, %2383 : i1
    acir.effect %2740(%0) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire", "top.rob.slots.2:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2740(%2455) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc171)
    acir.effect %2740(%0) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2741 = arith.andi %2738, %2375 : i1
    acir.effect %2741(%0) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire", "top.rob.slots.1:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2741(%2455) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc171)
    acir.effect %2741(%0) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2742 = arith.andi %2738, %2336 : i1
    acir.effect %2742(%0) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire", "top.rob.slots.0:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2742(%2455) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc171)
    acir.effect %2742(%0) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2738(%2695) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2738(%2699) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "write", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1, i32) -> () loc(#loc173)
    acir.effect %2738(%0) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2743 = arith.andi %2738, %2702 : i1
    acir.effect %2743(%2716) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1, !acir.array<i32, [8]>) -> () loc(#loc177)
    acir.effect %2743(%2730) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1, !acir.array<i32, [8]>) -> () loc(#loc180)
    acir.effect %2738(%2735) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1, i32) -> () loc(#loc185)
    acir.effect %2738(%2736) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.last_tag", txn = 7 : i64} : (i1, i32) -> () loc(#loc184)
    acir.path %true {branch = 59 : i64, kind = "branch", txn = 8 : i64} loc(#loc186)
    acir.path %true {branch = 59 : i64, frames = ["top.audit:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 8 : i64} loc(#loc186)
    %value_146, %ready_147 = acir.read(%true) {branch = 59 : i64, frames = ["top.audit:observe"], method = "get", port = 0 : i64, resource = "top.event", txn = 8 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc187)
    %2744 = acir.extract %value_146 {acir.id = "v6", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc188)
    %2745 = acir.fire "top.audit.observe#8" %ready_147 {successors = [], txn = 8 : i64}
    acir.effect %2745(%2744) {branch = 59 : i64, frames = ["top.audit:observe"], method = "write", port = 0 : i64, resource = "top.audit.last_tag", txn = 8 : i64} : (i1, i32) -> () loc(#loc189)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:13)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":46:16)
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
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:20)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":21:17)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:16)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:32)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:55)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:75)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:20)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:17)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:16)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:9)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:16)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:45)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:20)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:16)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":45:17)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:27)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:38)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:48)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:66)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:16)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:16)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:20)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":79:13)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:19)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:9)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:9)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:9)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:12)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":70:17)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:25)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:13)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:9)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:27)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:38)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:48)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:66)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:16)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:16)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:22)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:16)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:15)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:9)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:17)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:12)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:33)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:29)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:50)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:13)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":67:13)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:17)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:12)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:20)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:16)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:40)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:29)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:17)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:20)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:16)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:40)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:29)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:17)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":42:13)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:16)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:9)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:12)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":136:15)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:20)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:19)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:12)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:39)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:20)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":139:17)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:23)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:22)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:16)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:12)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:28)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:44)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:39)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:20)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:16)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:16)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:23)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:20)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:15)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:16)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:15)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:9)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:20)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:16)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":58:20)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:9)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:16)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:9)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:16)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:9)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:12)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:13)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:20)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:16)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":15:20)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":17:9)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":70:9)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":64:17)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:15)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:24)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:23)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:17)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:17)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:16)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:32)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:48)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":79:20)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":81:17)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:17)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:9)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:16)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:9)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:9)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":152:12)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:35)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:13)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:23)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:35)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:13)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:23)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:19)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:20)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:9)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":80:17)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:20)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:9)

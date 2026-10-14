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
    acir.state @top.rob.slot0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slot1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slot2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slot3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
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
    %658 = arith.andi %657, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc67)
    %659 = arith.cmpi eq, %658, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc68)
    %660 = arith.andi %650, %653 : i1
    %661 = arith.cmpi ne, %658, %c0_i32 : i32 loc(#loc68)
    %662 = arith.cmpi eq, %658, %c0_i32 : i32 loc(#loc68)
    %663 = arith.ori %662, %653 : i1
    %664 = arith.cmpi eq, %658, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc69)
    %665 = arith.andi %663, %653 : i1
    %666 = arith.ori %662, %665 : i1
    %667 = arith.andi %660, %666 : i1
    %668 = arith.andi %661, %664 : i1
    %669 = arith.cmpi ne, %658, %c1_i32 : i32 loc(#loc69)
    %670 = arith.andi %661, %669 : i1
    %671 = arith.xori %670, %true : i1
    %672 = arith.ori %671, %665 : i1
    %673 = arith.cmpi eq, %658, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc70)
    %674 = arith.andi %672, %653 : i1
    %675 = arith.ori %671, %674 : i1
    %676 = arith.andi %667, %675 : i1
    %677 = arith.andi %670, %673 : i1
    %678 = arith.cmpi ne, %658, %c2_i32 : i32 loc(#loc70)
    %679 = arith.andi %670, %678 : i1
    %680 = arith.xori %679, %true : i1 loc(#loc71)
    %681 = arith.ori %680, %674 : i1
    acir.path %679 {branch = 19 : i64, kind = "branch", txn = 5 : i64} loc(#loc71)
    acir.path %679 {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc71)
    %682 = arith.ori %680, %681 : i1 loc(#loc71)
    %683 = arith.andi %681, %653 : i1 loc(#loc71)
    %684 = arith.andi %679, %683 : i1 loc(#loc71)
    %value_52, %ready_53 = acir.read(%679) {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc72)
    %685 = arith.andi %682, %ready_53 : i1 loc(#loc72)
    %686 = arith.ori %680, %685 : i1 loc(#loc72)
    %687 = arith.andi %676, %686 : i1 loc(#loc72)
    %688 = acir.extract %value_52 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc73)
    %689 = arith.andi %687, %686 : i1
    %690 = arith.andi %679, %688 : i1
    %691 = arith.xori %688, %true : i1
    %692 = arith.andi %679, %691 : i1
    %693 = arith.xori %692, %true : i1
    %694 = arith.ori %693, %685 : i1
    %695 = arith.xori %690, %true : i1
    %696 = arith.ori %695, %685 : i1
    %697 = acir.extract %value_52 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc74)
    %698 = acir.unary %697 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc75)
    %699 = arith.andi %692, %694 : i1
    %700 = arith.andi %696, %ready_53 : i1
    %701 = arith.andi %690, %700 : i1
    %702 = arith.ori %699, %701 : i1
    %703 = arith.select %692, %false, %698 : i1
    %704 = arith.ori %680, %682 : i1
    %705 = arith.andi %704, %702 : i1
    %706 = arith.ori %680, %705 : i1
    %707 = arith.andi %689, %706 : i1
    %708 = arith.andi %679, %703 : i1
    %709 = arith.xori %703, %true : i1
    %710 = arith.andi %679, %709 : i1
    %711 = arith.xori %710, %true : i1
    %712 = arith.ori %711, %705 : i1
    %713 = arith.xori %708, %true : i1
    %714 = arith.ori %713, %705 : i1
    %715 = acir.extract %value_52 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %716 = arith.andi %ready_53, %684 : i1 loc(#loc76)
    %717 = arith.cmpi eq, %715, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc76)
    %718 = arith.andi %710, %712 : i1
    %719 = arith.andi %714, %716 : i1
    %720 = arith.andi %708, %719 : i1
    %721 = arith.ori %718, %720 : i1
    %722 = arith.select %710, %false, %717 : i1
    %723 = arith.ori %680, %704 : i1
    %724 = arith.andi %723, %721 : i1
    %725 = arith.ori %680, %724 : i1
    %726 = arith.andi %707, %725 : i1
    %727 = arith.andi %679, %722 : i1
    %728 = arith.xori %727, %true : i1 loc(#loc77)
    %729 = arith.ori %728, %724 : i1
    %730 = acir.extract %value_52 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc78)
    %731 = acir.extract %value_52 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %732 = acir.extract %value_52 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %733 = acir.extract %value_52 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %734 = acir.aggregate(%730, %true, %731, %732, %733) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc78)
    %735 = acir.extract %734 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %736 = acir.extract %734 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %737 = acir.extract %734 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %738 = acir.extract %734 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %739 = acir.aggregate(%735, %736, %737, %738, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc79)
    %740 = arith.andi %729, %716 : i1 loc(#loc77)
    %741 = arith.ori %728, %740 : i1 loc(#loc77)
    %742 = arith.andi %726, %741 : i1 loc(#loc77)
    acir.path %727 {branch = 20 : i64, kind = "branch", txn = 5 : i64} loc(#loc77)
    %743 = arith.xori %677, %true : i1 loc(#loc80)
    %744 = arith.ori %743, %674 : i1
    acir.path %677 {branch = 21 : i64, kind = "branch", txn = 5 : i64} loc(#loc80)
    acir.path %677 {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc80)
    %745 = arith.ori %743, %744 : i1 loc(#loc80)
    %746 = arith.andi %744, %653 : i1 loc(#loc80)
    %747 = arith.andi %677, %746 : i1 loc(#loc80)
    %value_54, %ready_55 = acir.read(%677) {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc72)
    %748 = arith.andi %745, %ready_55 : i1 loc(#loc72)
    %749 = arith.ori %743, %748 : i1 loc(#loc72)
    %750 = arith.andi %742, %749 : i1 loc(#loc72)
    %751 = acir.extract %value_54 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc73)
    %752 = arith.andi %750, %749 : i1
    %753 = arith.andi %677, %751 : i1
    %754 = arith.xori %751, %true : i1
    %755 = arith.andi %677, %754 : i1
    %756 = arith.xori %755, %true : i1
    %757 = arith.ori %756, %748 : i1
    %758 = arith.xori %753, %true : i1
    %759 = arith.ori %758, %748 : i1
    %760 = acir.extract %value_54 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc74)
    %761 = acir.unary %760 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc75)
    %762 = arith.andi %755, %757 : i1
    %763 = arith.andi %759, %ready_55 : i1
    %764 = arith.andi %753, %763 : i1
    %765 = arith.ori %762, %764 : i1
    %766 = arith.select %755, %false, %761 : i1
    %767 = arith.ori %743, %745 : i1
    %768 = arith.andi %767, %765 : i1
    %769 = arith.ori %743, %768 : i1
    %770 = arith.andi %752, %769 : i1
    %771 = arith.andi %677, %766 : i1
    %772 = arith.xori %766, %true : i1
    %773 = arith.andi %677, %772 : i1
    %774 = arith.xori %773, %true : i1
    %775 = arith.ori %774, %768 : i1
    %776 = arith.xori %771, %true : i1
    %777 = arith.ori %776, %768 : i1
    %778 = acir.extract %value_54 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %779 = arith.andi %ready_55, %747 : i1 loc(#loc76)
    %780 = arith.cmpi eq, %778, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc76)
    %781 = arith.andi %773, %775 : i1
    %782 = arith.andi %777, %779 : i1
    %783 = arith.andi %771, %782 : i1
    %784 = arith.ori %781, %783 : i1
    %785 = arith.select %773, %false, %780 : i1
    %786 = arith.ori %743, %767 : i1
    %787 = arith.andi %786, %784 : i1
    %788 = arith.ori %743, %787 : i1
    %789 = arith.andi %770, %788 : i1
    %790 = arith.andi %677, %785 : i1
    %791 = arith.xori %790, %true : i1 loc(#loc77)
    %792 = arith.ori %791, %787 : i1
    %793 = acir.extract %value_54 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc78)
    %794 = acir.extract %value_54 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %795 = acir.extract %value_54 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %796 = acir.extract %value_54 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %797 = acir.aggregate(%793, %true, %794, %795, %796) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc78)
    %798 = acir.extract %797 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %799 = acir.extract %797 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %800 = acir.extract %797 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %801 = acir.extract %797 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %802 = acir.aggregate(%798, %799, %800, %801, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc79)
    %803 = arith.andi %792, %779 : i1 loc(#loc77)
    %804 = arith.ori %791, %803 : i1 loc(#loc77)
    %805 = arith.andi %789, %804 : i1 loc(#loc77)
    acir.path %790 {branch = 22 : i64, kind = "branch", txn = 5 : i64} loc(#loc77)
    %806 = arith.xori %668, %true : i1 loc(#loc81)
    %807 = arith.ori %806, %665 : i1
    acir.path %668 {branch = 23 : i64, kind = "branch", txn = 5 : i64} loc(#loc81)
    acir.path %668 {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc81)
    %808 = arith.ori %806, %807 : i1 loc(#loc81)
    %809 = arith.andi %807, %653 : i1 loc(#loc81)
    %810 = arith.andi %668, %809 : i1 loc(#loc81)
    %value_56, %ready_57 = acir.read(%668) {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc72)
    %811 = arith.andi %808, %ready_57 : i1 loc(#loc72)
    %812 = arith.ori %806, %811 : i1 loc(#loc72)
    %813 = arith.andi %805, %812 : i1 loc(#loc72)
    %814 = acir.extract %value_56 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc73)
    %815 = arith.andi %813, %812 : i1
    %816 = arith.andi %668, %814 : i1
    %817 = arith.xori %814, %true : i1
    %818 = arith.andi %668, %817 : i1
    %819 = arith.xori %818, %true : i1
    %820 = arith.ori %819, %811 : i1
    %821 = arith.xori %816, %true : i1
    %822 = arith.ori %821, %811 : i1
    %823 = acir.extract %value_56 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc74)
    %824 = acir.unary %823 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc75)
    %825 = arith.andi %818, %820 : i1
    %826 = arith.andi %822, %ready_57 : i1
    %827 = arith.andi %816, %826 : i1
    %828 = arith.ori %825, %827 : i1
    %829 = arith.select %818, %false, %824 : i1
    %830 = arith.ori %806, %808 : i1
    %831 = arith.andi %830, %828 : i1
    %832 = arith.ori %806, %831 : i1
    %833 = arith.andi %815, %832 : i1
    %834 = arith.andi %668, %829 : i1
    %835 = arith.xori %829, %true : i1
    %836 = arith.andi %668, %835 : i1
    %837 = arith.xori %836, %true : i1
    %838 = arith.ori %837, %831 : i1
    %839 = arith.xori %834, %true : i1
    %840 = arith.ori %839, %831 : i1
    %841 = acir.extract %value_56 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %842 = arith.andi %ready_57, %810 : i1 loc(#loc76)
    %843 = arith.cmpi eq, %841, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc76)
    %844 = arith.andi %836, %838 : i1
    %845 = arith.andi %840, %842 : i1
    %846 = arith.andi %834, %845 : i1
    %847 = arith.ori %844, %846 : i1
    %848 = arith.select %836, %false, %843 : i1
    %849 = arith.ori %806, %830 : i1
    %850 = arith.andi %849, %847 : i1
    %851 = arith.ori %806, %850 : i1
    %852 = arith.andi %833, %851 : i1
    %853 = arith.andi %668, %848 : i1
    %854 = arith.xori %853, %true : i1 loc(#loc77)
    %855 = arith.ori %854, %850 : i1
    %856 = acir.extract %value_56 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc78)
    %857 = acir.extract %value_56 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %858 = acir.extract %value_56 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %859 = acir.extract %value_56 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %860 = acir.aggregate(%856, %true, %857, %858, %859) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc78)
    %861 = acir.extract %860 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %862 = acir.extract %860 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %863 = acir.extract %860 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %864 = acir.extract %860 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %865 = acir.aggregate(%861, %862, %863, %864, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc79)
    %866 = arith.andi %855, %842 : i1 loc(#loc77)
    %867 = arith.ori %854, %866 : i1 loc(#loc77)
    %868 = arith.andi %852, %867 : i1 loc(#loc77)
    acir.path %853 {branch = 24 : i64, kind = "branch", txn = 5 : i64} loc(#loc77)
    %869 = arith.ori %661, %653 : i1
    acir.path %659 {branch = 25 : i64, kind = "branch", txn = 5 : i64} loc(#loc82)
    acir.path %659 {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc82)
    %870 = arith.ori %661, %869 : i1 loc(#loc82)
    %871 = arith.andi %869, %653 : i1 loc(#loc82)
    %872 = arith.andi %659, %871 : i1 loc(#loc82)
    %value_58, %ready_59 = acir.read(%659) {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc72)
    %873 = arith.andi %870, %ready_59 : i1 loc(#loc72)
    %874 = arith.ori %661, %873 : i1 loc(#loc72)
    %875 = arith.andi %868, %874 : i1 loc(#loc72)
    %876 = acir.extract %value_58 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc73)
    %877 = arith.andi %875, %874 : i1
    %878 = arith.andi %659, %876 : i1
    %879 = arith.xori %876, %true : i1
    %880 = arith.andi %659, %879 : i1
    %881 = arith.xori %880, %true : i1
    %882 = arith.ori %881, %873 : i1
    %883 = arith.xori %878, %true : i1
    %884 = arith.ori %883, %873 : i1
    %885 = acir.extract %value_58 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc74)
    %886 = acir.unary %885 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc75)
    %887 = arith.andi %880, %882 : i1
    %888 = arith.andi %884, %ready_59 : i1
    %889 = arith.andi %878, %888 : i1
    %890 = arith.ori %887, %889 : i1
    %891 = arith.select %880, %false, %886 : i1
    %892 = arith.ori %661, %870 : i1
    %893 = arith.andi %892, %890 : i1
    %894 = arith.ori %661, %893 : i1
    %895 = arith.andi %877, %894 : i1
    %896 = arith.andi %659, %891 : i1
    %897 = arith.xori %891, %true : i1
    %898 = arith.andi %659, %897 : i1
    %899 = arith.xori %898, %true : i1
    %900 = arith.ori %899, %893 : i1
    %901 = arith.xori %896, %true : i1
    %902 = arith.ori %901, %893 : i1
    %903 = acir.extract %value_58 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc76)
    %904 = arith.andi %ready_59, %872 : i1 loc(#loc76)
    %905 = arith.cmpi eq, %903, %655 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc76)
    %906 = arith.andi %898, %900 : i1
    %907 = arith.andi %902, %904 : i1
    %908 = arith.andi %896, %907 : i1
    %909 = arith.ori %906, %908 : i1
    %910 = arith.select %898, %false, %905 : i1
    %911 = arith.ori %661, %892 : i1
    %912 = arith.andi %911, %909 : i1
    %913 = arith.ori %661, %912 : i1
    %914 = arith.andi %895, %913 : i1
    %915 = arith.andi %659, %910 : i1
    %916 = arith.xori %915, %true : i1 loc(#loc77)
    %917 = arith.ori %916, %912 : i1
    %918 = acir.extract %value_58 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc78)
    %919 = acir.extract %value_58 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %920 = acir.extract %value_58 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %921 = acir.extract %value_58 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc78)
    %922 = acir.aggregate(%918, %true, %919, %920, %921) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc78)
    %923 = acir.extract %922 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %924 = acir.extract %922 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc79)
    %925 = acir.extract %922 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %926 = acir.extract %922 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc79)
    %927 = acir.aggregate(%923, %924, %925, %926, %656) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc79)
    %928 = arith.andi %917, %904 : i1 loc(#loc77)
    %929 = arith.ori %916, %928 : i1 loc(#loc77)
    %930 = arith.andi %914, %929 : i1 loc(#loc77)
    acir.path %915 {branch = 26 : i64, kind = "branch", txn = 5 : i64} loc(#loc77)
    %931 = acir.extract %654 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc83)
    %932 = acir.extract %654 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc84)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc85)
    %value_60, %ready_61 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc86)
    %933 = arith.andi %930, %ready_61 : i1 loc(#loc86)
    %934 = acir.extract %value_60 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc87)
    %935 = arith.xori %934, %true : i1
    %936 = arith.ori %934, %ready_61 : i1
    %937 = arith.ori %935, %ready_61 : i1
    %938 = acir.extract %value_60 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc88)
    %939 = acir.extract %938 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc88)
    %940 = acir.unary %939 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc89)
    %941 = arith.andi %937, %ready_61 : i1
    %942 = arith.ori %935, %941 : i1
    %943 = arith.andi %933, %942 : i1
    %944 = arith.andi %934, %940 : i1
    %945 = arith.xori %940, %true : i1
    %946 = arith.andi %934, %945 : i1
    %947 = arith.xori %946, %true : i1
    %948 = arith.ori %947, %941 : i1
    %949 = arith.xori %944, %true : i1
    %950 = arith.ori %949, %941 : i1
    %951 = acir.extract %value_60 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc90)
    %952 = acir.extract %951 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc90)
    %953 = arith.andi %ready_61, %653 : i1 loc(#loc90)
    %954 = arith.cmpi eq, %952, %931 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc90)
    %955 = arith.andi %946, %948 : i1
    %956 = arith.andi %950, %953 : i1
    %957 = arith.andi %944, %956 : i1
    %958 = arith.ori %955, %957 : i1
    %959 = arith.select %946, %false, %954 : i1
    %960 = arith.ori %935, %937 : i1
    %961 = arith.andi %960, %958 : i1
    %962 = arith.ori %935, %961 : i1
    %963 = arith.andi %943, %962 : i1
    %964 = arith.andi %934, %959 : i1
    %965 = arith.xori %959, %true : i1
    %966 = arith.andi %934, %965 : i1
    %967 = arith.xori %966, %true : i1
    %968 = arith.ori %967, %961 : i1
    %969 = arith.xori %964, %true : i1
    %970 = arith.ori %969, %961 : i1
    %971 = acir.aggregate(%true, %931, %932) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc91)
    %972 = acir.extract %value_60 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc92)
    %973 = acir.extract %value_60 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %974 = acir.extract %value_60 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %975 = acir.extract %value_60 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %976 = acir.extract %value_60 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc92)
    %977 = acir.aggregate(%972, %973, %974, %975, %971, %976) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %978 = arith.andi %966, %968 : i1
    %979 = arith.andi %964, %970 : i1
    %980 = arith.ori %978, %979 : i1
    %981 = arith.andi %968, %ready_61 : i1
    %982 = arith.andi %966, %981 : i1
    %983 = arith.andi %970, %953 : i1
    %984 = arith.andi %964, %983 : i1
    %985 = arith.ori %982, %984 : i1
    %986 = arith.select %966, %value_60, %977 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %987 = arith.ori %935, %960 : i1
    %988 = acir.extract %986 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc93)
    %989 = acir.extract %988 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc93)
    %990 = acir.unary %989 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc94)
    %991 = arith.andi %987, %985 : i1
    %992 = arith.ori %935, %991 : i1
    %993 = arith.andi %963, %992 : i1
    %994 = arith.andi %934, %990 : i1
    %995 = arith.xori %990, %true : i1
    %996 = arith.andi %934, %995 : i1
    %997 = arith.xori %996, %true : i1
    %998 = arith.ori %997, %991 : i1
    %999 = arith.xori %994, %true : i1
    %1000 = arith.ori %999, %991 : i1
    %1001 = acir.extract %986 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc95)
    %1002 = acir.extract %1001 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc95)
    %1003 = arith.andi %985, %653 : i1 loc(#loc95)
    %1004 = arith.cmpi eq, %1002, %931 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc95)
    %1005 = arith.andi %996, %998 : i1
    %1006 = arith.andi %1000, %1003 : i1
    %1007 = arith.andi %994, %1006 : i1
    %1008 = arith.ori %1005, %1007 : i1
    %1009 = arith.select %996, %false, %1004 : i1
    %1010 = arith.ori %935, %987 : i1
    %1011 = arith.andi %1010, %1008 : i1
    %1012 = arith.ori %935, %1011 : i1
    %1013 = arith.andi %993, %1012 : i1
    %1014 = arith.andi %934, %1009 : i1
    %1015 = arith.xori %1009, %true : i1
    %1016 = arith.andi %934, %1015 : i1
    %1017 = arith.xori %1016, %true : i1
    %1018 = arith.ori %1017, %1011 : i1
    %1019 = arith.xori %1014, %true : i1
    %1020 = arith.ori %1019, %1011 : i1
    %1021 = acir.aggregate(%true, %931, %932) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc96)
    %1022 = acir.extract %986 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc97)
    %1023 = acir.extract %986 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc97)
    %1024 = acir.extract %986 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc97)
    %1025 = acir.extract %986 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc97)
    %1026 = acir.extract %986 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc97)
    %1027 = acir.aggregate(%1022, %1023, %1024, %1025, %1026, %1021) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc97)
    %1028 = arith.andi %1018, %980 : i1
    %1029 = arith.andi %1016, %1028 : i1
    %1030 = arith.andi %1014, %1020 : i1
    %1031 = arith.ori %1029, %1030 : i1
    %1032 = arith.select %1016, %967, %true : i1
    %1033 = arith.andi %1018, %985 : i1
    %1034 = arith.andi %1016, %1033 : i1
    %1035 = arith.andi %1020, %1003 : i1
    %1036 = arith.andi %1014, %1035 : i1
    %1037 = arith.ori %1034, %1036 : i1
    %1038 = arith.select %1016, %986, %1027 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1039 = arith.ori %935, %1010 : i1
    %1040 = arith.andi %935, %936 : i1
    %1041 = arith.andi %1039, %1031 : i1
    %1042 = arith.andi %934, %1041 : i1
    %1043 = arith.ori %1040, %1042 : i1
    %1044 = arith.select %934, %1032, %false : i1
    %1045 = arith.andi %936, %ready_61 : i1
    %1046 = arith.andi %935, %1045 : i1
    %1047 = arith.andi %1039, %1037 : i1
    %1048 = arith.andi %934, %1047 : i1
    %1049 = arith.ori %1046, %1048 : i1
    %1050 = arith.select %934, %1038, %value_60 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1051 = arith.andi %1013, %1043 : i1
    %1052 = arith.xori %1044, %true : i1 loc(#loc98)
    %1053 = arith.ori %1052, %1043 : i1
    %1054 = arith.andi %1053, %1049 : i1 loc(#loc98)
    %1055 = arith.ori %1052, %1054 : i1 loc(#loc98)
    %1056 = arith.andi %1051, %1055 : i1 loc(#loc98)
    acir.path %1044 {branch = 27 : i64, kind = "branch", txn = 5 : i64} loc(#loc98)
    %1057 = acir.extract %654 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc99)
    %1058 = acir.extract %654 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc100)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc101)
    %value_62, %ready_63 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc86)
    %1059 = arith.andi %1056, %ready_63 : i1 loc(#loc86)
    %1060 = acir.extract %value_62 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc87)
    %1061 = arith.xori %1060, %true : i1
    %1062 = arith.ori %1060, %ready_63 : i1
    %1063 = arith.ori %1061, %ready_63 : i1
    %1064 = acir.extract %value_62 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc88)
    %1065 = acir.extract %1064 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc88)
    %1066 = acir.unary %1065 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc89)
    %1067 = arith.andi %1063, %ready_63 : i1
    %1068 = arith.ori %1061, %1067 : i1
    %1069 = arith.andi %1059, %1068 : i1
    %1070 = arith.andi %1060, %1066 : i1
    %1071 = arith.xori %1066, %true : i1
    %1072 = arith.andi %1060, %1071 : i1
    %1073 = arith.xori %1072, %true : i1
    %1074 = arith.ori %1073, %1067 : i1
    %1075 = arith.xori %1070, %true : i1
    %1076 = arith.ori %1075, %1067 : i1
    %1077 = acir.extract %value_62 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc90)
    %1078 = acir.extract %1077 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc90)
    %1079 = arith.andi %ready_63, %653 : i1 loc(#loc90)
    %1080 = arith.cmpi eq, %1078, %1057 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc90)
    %1081 = arith.andi %1072, %1074 : i1
    %1082 = arith.andi %1076, %1079 : i1
    %1083 = arith.andi %1070, %1082 : i1
    %1084 = arith.ori %1081, %1083 : i1
    %1085 = arith.select %1072, %false, %1080 : i1
    %1086 = arith.ori %1061, %1063 : i1
    %1087 = arith.andi %1086, %1084 : i1
    %1088 = arith.ori %1061, %1087 : i1
    %1089 = arith.andi %1069, %1088 : i1
    %1090 = arith.andi %1060, %1085 : i1
    %1091 = arith.xori %1085, %true : i1
    %1092 = arith.andi %1060, %1091 : i1
    %1093 = arith.xori %1092, %true : i1
    %1094 = arith.ori %1093, %1087 : i1
    %1095 = arith.xori %1090, %true : i1
    %1096 = arith.ori %1095, %1087 : i1
    %1097 = acir.aggregate(%true, %1057, %1058) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc91)
    %1098 = acir.extract %value_62 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc92)
    %1099 = acir.extract %value_62 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1100 = acir.extract %value_62 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1101 = acir.extract %value_62 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc92)
    %1102 = acir.extract %value_62 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc92)
    %1103 = acir.aggregate(%1098, %1099, %1100, %1101, %1097, %1102) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %1104 = arith.andi %1092, %1094 : i1
    %1105 = arith.andi %1090, %1096 : i1
    %1106 = arith.ori %1104, %1105 : i1
    %1107 = arith.andi %1094, %ready_63 : i1
    %1108 = arith.andi %1092, %1107 : i1
    %1109 = arith.andi %1096, %1079 : i1
    %1110 = arith.andi %1090, %1109 : i1
    %1111 = arith.ori %1108, %1110 : i1
    %1112 = arith.select %1092, %value_62, %1103 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1113 = arith.ori %1061, %1086 : i1
    %1114 = acir.extract %1112 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc93)
    %1115 = acir.extract %1114 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc93)
    %1116 = acir.unary %1115 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc94)
    %1117 = arith.andi %1113, %1111 : i1
    %1118 = arith.ori %1061, %1117 : i1
    %1119 = arith.andi %1089, %1118 : i1
    %1120 = arith.andi %1060, %1116 : i1
    %1121 = arith.xori %1116, %true : i1
    %1122 = arith.andi %1060, %1121 : i1
    %1123 = arith.xori %1122, %true : i1
    %1124 = arith.ori %1123, %1117 : i1
    %1125 = arith.xori %1120, %true : i1
    %1126 = arith.ori %1125, %1117 : i1
    %1127 = acir.extract %1112 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc95)
    %1128 = acir.extract %1127 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc95)
    %1129 = arith.andi %1111, %653 : i1 loc(#loc95)
    %1130 = arith.cmpi eq, %1128, %1057 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc95)
    %1131 = arith.andi %1122, %1124 : i1
    %1132 = arith.andi %1126, %1129 : i1
    %1133 = arith.andi %1120, %1132 : i1
    %1134 = arith.ori %1131, %1133 : i1
    %1135 = arith.select %1122, %false, %1130 : i1
    %1136 = arith.ori %1061, %1113 : i1
    %1137 = arith.andi %1136, %1134 : i1
    %1138 = arith.ori %1061, %1137 : i1
    %1139 = arith.andi %1119, %1138 : i1
    %1140 = arith.andi %1060, %1135 : i1
    %1141 = arith.xori %1135, %true : i1
    %1142 = arith.andi %1060, %1141 : i1
    %1143 = arith.xori %1142, %true : i1
    %1144 = arith.ori %1143, %1137 : i1
    %1145 = arith.xori %1140, %true : i1
    %1146 = arith.ori %1145, %1137 : i1
    %1147 = acir.aggregate(%true, %1057, %1058) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc96)
    %1148 = acir.extract %1112 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc97)
    %1149 = acir.extract %1112 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc97)
    %1150 = acir.extract %1112 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc97)
    %1151 = acir.extract %1112 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc97)
    %1152 = acir.extract %1112 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc97)
    %1153 = acir.aggregate(%1148, %1149, %1150, %1151, %1152, %1147) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc97)
    %1154 = arith.andi %1144, %1106 : i1
    %1155 = arith.andi %1142, %1154 : i1
    %1156 = arith.andi %1140, %1146 : i1
    %1157 = arith.ori %1155, %1156 : i1
    %1158 = arith.select %1142, %1093, %true : i1
    %1159 = arith.andi %1144, %1111 : i1
    %1160 = arith.andi %1142, %1159 : i1
    %1161 = arith.andi %1146, %1129 : i1
    %1162 = arith.andi %1140, %1161 : i1
    %1163 = arith.ori %1160, %1162 : i1
    %1164 = arith.select %1142, %1112, %1153 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1165 = arith.ori %1061, %1136 : i1
    %1166 = arith.andi %1061, %1062 : i1
    %1167 = arith.andi %1165, %1157 : i1
    %1168 = arith.andi %1060, %1167 : i1
    %1169 = arith.ori %1166, %1168 : i1
    %1170 = arith.select %1060, %1158, %false : i1
    %1171 = arith.andi %1062, %ready_63 : i1
    %1172 = arith.andi %1061, %1171 : i1
    %1173 = arith.andi %1165, %1163 : i1
    %1174 = arith.andi %1060, %1173 : i1
    %1175 = arith.ori %1172, %1174 : i1
    %1176 = arith.select %1060, %1164, %value_62 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1177 = arith.andi %1139, %1169 : i1
    %1178 = arith.xori %1170, %true : i1 loc(#loc98)
    %1179 = arith.ori %1178, %1169 : i1
    %1180 = arith.andi %1179, %1175 : i1 loc(#loc98)
    %1181 = arith.ori %1178, %1180 : i1 loc(#loc98)
    %1182 = arith.andi %1177, %1181 : i1 loc(#loc98)
    acir.path %1170 {branch = 28 : i64, kind = "branch", txn = 5 : i64} loc(#loc98)
    acir.path %653 {branch = 29 : i64, kind = "branch", txn = 5 : i64}
    %1183 = acir.fire "top.cdb.broadcast#5" %1182 {successors = [2, 4, 8], txn = 5 : i64}
    %1184 = arith.andi %1183, %626 : i1
    acir.effect %1184() {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> () loc(#loc63)
    %1185 = arith.andi %1183, %625 : i1
    acir.effect %1185() {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> () loc(#loc62)
    %1186 = arith.andi %1183, %727 : i1
    acir.effect %1186(%739) {branch = 20 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc77)
    %1187 = arith.andi %1183, %790 : i1
    acir.effect %1187(%802) {branch = 22 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc77)
    %1188 = arith.andi %1183, %853 : i1
    acir.effect %1188(%865) {branch = 24 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc77)
    %1189 = arith.andi %1183, %915 : i1
    acir.effect %1189(%927) {branch = 26 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc77)
    %1190 = arith.andi %1183, %1044 : i1
    acir.effect %1190(%1050) {branch = 27 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc98)
    %1191 = arith.andi %1183, %1170 : i1
    acir.effect %1191(%1176) {branch = 28 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc98)
    %1192 = arith.andi %1183, %653 : i1
    acir.effect %1192(%654) {branch = 29 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.event", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> ()
    %value_64, %ready_65 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.add_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc102)
    %1193 = arith.xori %value_64, %true : i1
    %1194 = arith.ori %value_64, %ready_65 : i1
    %1195 = arith.ori %1193, %ready_65 : i1
    acir.path %value_64 {branch = 30 : i64, kind = "branch", txn = -1 : i64} loc(#loc103)
    %value_66, %ready_67 = acir.read(%value_64) {branch = 30 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.mul_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc103)
    %1196 = arith.andi %1193, %1194 : i1
    %1197 = arith.andi %1195, %ready_67 : i1
    %1198 = arith.andi %value_64, %1197 : i1
    %1199 = arith.ori %1196, %1198 : i1
    %1200 = arith.select %value_64, %value_66, %false : i1
    %1201 = arith.xori %1200, %true : i1 loc(#loc104)
    %1202 = arith.ori %1201, %1199 : i1
    %1203 = arith.andi %1200, %1202 : i1 loc(#loc104)
    acir.path %1200 {branch = 31 : i64, kind = "branch", txn = 6 : i64} loc(#loc104)
    acir.path %1200 {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc104)
    %1204 = arith.ori %1201, %1202 : i1 loc(#loc104)
    %value_68, %ready_69 = acir.read(%1200) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "peek", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, i1) loc(#loc105)
    %1205 = arith.andi %1204, %ready_69 : i1 loc(#loc105)
    %1206 = arith.ori %1201, %1205 : i1 loc(#loc105)
    %1207 = arith.andi %1203, %1206 : i1 loc(#loc105)
    %1208 = arith.andi %1207, %1206 : i1 loc(#loc105)
    %value_70, %ready_71 = acir.read(%1200) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "empty", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (i1, i1) loc(#loc105)
    %1209 = arith.andi %1204, %ready_71 : i1 loc(#loc105)
    %1210 = arith.ori %1201, %1209 : i1 loc(#loc105)
    %1211 = arith.andi %1208, %1210 : i1 loc(#loc105)
    %1212 = arith.xori %value_70, %true : i1 loc(#loc105)
    %1213 = arith.andi %1204, %1212 : i1 loc(#loc105)
    %1214 = arith.ori %1201, %1213 : i1 loc(#loc105)
    %1215 = arith.andi %1211, %1214 : i1 loc(#loc105)
    %1216 = acir.extract %value_68 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc106)
    acir.path %1200 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc107)
    %1217 = arith.ori %1201, %1204 : i1 loc(#loc107)
    %1218 = arith.andi %1200, %1205 : i1 loc(#loc107)
    %1219 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1220 = arith.cmpi ult, %1216, %c8_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc108)
    %1221 = arith.andi %1217, %1218 : i1 loc(#loc109)
    %1222 = arith.ori %1201, %1221 : i1 loc(#loc109)
    %1223 = arith.andi %1215, %1222 : i1 loc(#loc109)
    %1224 = arith.cmpi eq, %1216, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc110)
    %1225 = arith.andi %1223, %1222 : i1
    %1226 = arith.andi %1200, %1224 : i1
    %1227 = arith.cmpi ne, %1216, %c0_i32 : i32 loc(#loc110)
    %1228 = arith.andi %1200, %1227 : i1
    %1229 = arith.xori %1228, %true : i1 loc(#loc111)
    %1230 = arith.ori %1229, %1221 : i1
    %1231 = arith.xori %1226, %true : i1
    %1232 = arith.ori %1231, %1221 : i1
    %1233 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc112)
    %1234 = arith.andi %1228, %1230 : i1
    %1235 = arith.andi %1226, %1232 : i1
    %1236 = arith.ori %1234, %1235 : i1
    %1237 = arith.select %1228, %1219, %1233 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1238 = arith.ori %1201, %1217 : i1 loc(#loc113)
    %1239 = arith.andi %1238, %1236 : i1
    %1240 = arith.ori %1201, %1239 : i1
    %1241 = arith.andi %1225, %1240 : i1
    %1242 = arith.andi %1200, %1229 : i1
    %1243 = arith.xori %1242, %true : i1
    %1244 = arith.ori %1243, %1239 : i1
    %1245 = arith.ori %1229, %1239 : i1
    acir.path %1228 {branch = 32 : i64, kind = "branch", txn = 6 : i64} loc(#loc111)
    %value_72, %ready_73 = acir.read(%1228) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc111)
    %1246 = arith.andi %1245, %ready_73 : i1 loc(#loc111)
    %1247 = arith.ori %1229, %1246 : i1 loc(#loc111)
    %1248 = arith.andi %1241, %1247 : i1 loc(#loc111)
    %1249 = arith.andi %ready_73, %1218 : i1 loc(#loc111)
    %1250 = arith.extui %1216 : i32 to i64 loc(#loc111)
    %1251 = arith.cmpi ult, %1250, %c8_i64 : i64 loc(#loc111)
    %1252 = arith.select %1251, %1250, %c0_i64 : i64 loc(#loc111)
    %1253 = arith.andi %1249, %1251 : i1 loc(#loc111)
    %1254 = "acir.array.index"(%value_72, %1252) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc111)
    %value_74, %ready_75 = acir.read(%1228) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc114)
    %1255 = arith.andi %1245, %ready_75 : i1 loc(#loc114)
    %1256 = arith.ori %1229, %1255 : i1 loc(#loc114)
    %1257 = arith.andi %1248, %1256 : i1 loc(#loc114)
    %1258 = arith.andi %ready_75, %1218 : i1 loc(#loc114)
    %1259 = arith.andi %1258, %1251 : i1 loc(#loc114)
    %1260 = "acir.array.index"(%value_74, %1252) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc114)
    %1261 = arith.andi %1253, %1259 : i1 loc(#loc115)
    %1262 = arith.cmpi eq, %1254, %1260 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc115)
    %1263 = arith.andi %1245, %1261 : i1
    %1264 = arith.ori %1229, %1263 : i1
    %1265 = arith.andi %1257, %1264 : i1
    %1266 = arith.andi %1228, %1262 : i1
    %1267 = arith.cmpi ne, %1254, %1260 : i32 loc(#loc115)
    %1268 = arith.andi %1228, %1267 : i1
    %1269 = arith.xori %1268, %true : i1
    %1270 = arith.ori %1269, %1263 : i1
    %1271 = arith.xori %1266, %true : i1 loc(#loc116)
    %1272 = arith.ori %1271, %1263 : i1
    acir.path %1266 {branch = 33 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    %value_76, %ready_77 = acir.read(%1266) {branch = 33 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc116)
    %1273 = arith.andi %1272, %ready_77 : i1 loc(#loc116)
    %1274 = arith.ori %1271, %1273 : i1 loc(#loc116)
    %1275 = arith.andi %1265, %1274 : i1 loc(#loc116)
    %1276 = arith.andi %ready_77, %1218 : i1 loc(#loc116)
    %1277 = arith.andi %1276, %1251 : i1 loc(#loc116)
    %1278 = "acir.array.index"(%value_76, %1252) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc116)
    %1279 = arith.andi %1253, %1277 : i1 loc(#loc117)
    %1280 = acir.aggregate(%true, %1254, %1278) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc117)
    %1281 = arith.andi %1270, %1236 : i1
    %1282 = arith.andi %1268, %1281 : i1
    %1283 = arith.andi %1266, %1272 : i1
    %1284 = arith.ori %1282, %1283 : i1
    %1285 = arith.select %1268, %1228, %false : i1
    %1286 = arith.andi %1272, %1279 : i1
    %1287 = arith.andi %1266, %1286 : i1
    %1288 = arith.ori %1282, %1287 : i1
    %1289 = arith.select %1268, %1237, %1280 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1290 = arith.ori %1229, %1245 : i1
    %1291 = arith.andi %1290, %1284 : i1
    %1292 = arith.ori %1229, %1291 : i1
    %1293 = arith.andi %1275, %1292 : i1
    %1294 = arith.andi %1228, %1285 : i1
    %1295 = arith.xori %1285, %true : i1
    %1296 = arith.andi %1228, %1295 : i1
    %1297 = arith.xori %1296, %true : i1
    %1298 = arith.ori %1297, %1291 : i1
    %1299 = arith.xori %1294, %true : i1 loc(#loc118)
    %1300 = arith.ori %1299, %1291 : i1
    acir.path %1294 {branch = 34 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %1294 {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc118)
    %1301 = arith.ori %1299, %1300 : i1 loc(#loc118)
    %1302 = arith.andi %1300, %1253 : i1 loc(#loc118)
    %1303 = arith.andi %1294, %1302 : i1 loc(#loc118)
    %1304 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1305 = arith.subi %1254, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc119)
    %1306 = arith.andi %1305, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc119)
    %1307 = arith.cmpi eq, %1306, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc120)
    %1308 = arith.andi %1301, %1303 : i1
    %1309 = arith.ori %1299, %1308 : i1
    %1310 = arith.andi %1293, %1309 : i1
    %1311 = arith.andi %1294, %1307 : i1
    %1312 = arith.cmpi ne, %1306, %c0_i32 : i32 loc(#loc120)
    %1313 = arith.andi %1294, %1312 : i1
    %1314 = arith.xori %1313, %true : i1
    %1315 = arith.ori %1314, %1308 : i1
    %1316 = arith.cmpi eq, %1306, %c1_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc121)
    %1317 = arith.andi %1315, %1303 : i1
    %1318 = arith.ori %1314, %1317 : i1
    %1319 = arith.andi %1310, %1318 : i1
    %1320 = arith.andi %1313, %1316 : i1
    %1321 = arith.cmpi ne, %1306, %c1_i32 : i32 loc(#loc121)
    %1322 = arith.andi %1313, %1321 : i1
    %1323 = arith.xori %1322, %true : i1
    %1324 = arith.ori %1323, %1317 : i1
    %1325 = arith.cmpi eq, %1306, %c2_i32 {acir.id = "v22", acir.source_type = "bool"} : i32 loc(#loc122)
    %1326 = arith.andi %1324, %1303 : i1
    %1327 = arith.ori %1323, %1326 : i1
    %1328 = arith.andi %1319, %1327 : i1
    %1329 = arith.andi %1322, %1325 : i1
    %1330 = arith.cmpi ne, %1306, %c2_i32 : i32 loc(#loc122)
    %1331 = arith.andi %1322, %1330 : i1
    %1332 = arith.xori %1331, %true : i1
    %1333 = arith.ori %1332, %1326 : i1
    %1334 = arith.xori %1329, %true : i1 loc(#loc123)
    %1335 = arith.ori %1334, %1326 : i1
    acir.path %1329 {branch = 35 : i64, kind = "branch", txn = 6 : i64} loc(#loc123)
    acir.path %1329 {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc123)
    %1336 = arith.ori %1334, %1335 : i1 loc(#loc123)
    %value_78, %ready_79 = acir.read(%1329) {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1337 = arith.andi %1336, %ready_79 : i1 loc(#loc124)
    %1338 = arith.ori %1334, %1337 : i1 loc(#loc124)
    %1339 = arith.andi %1328, %1338 : i1 loc(#loc124)
    %1340 = arith.ori %1334, %1336 : i1
    %1341 = arith.andi %1339, %1340 : i1
    %1342 = arith.andi %1329, %1337 : i1
    %1343 = arith.andi %1331, %1333 : i1
    %1344 = arith.andi %1329, %1335 : i1
    %1345 = arith.ori %1343, %1344 : i1
    %1346 = arith.andi %1335, %1342 : i1
    %1347 = arith.andi %1329, %1346 : i1
    %1348 = arith.ori %1343, %1347 : i1
    %1349 = arith.select %1331, %1304, %value_78 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1350 = arith.ori %1323, %1324 : i1
    %1351 = arith.xori %1320, %true : i1 loc(#loc125)
    %1352 = arith.ori %1351, %1317 : i1
    acir.path %1320 {branch = 36 : i64, kind = "branch", txn = 6 : i64} loc(#loc125)
    acir.path %1320 {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc125)
    %1353 = arith.ori %1351, %1352 : i1 loc(#loc125)
    %value_80, %ready_81 = acir.read(%1320) {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1354 = arith.andi %1353, %ready_81 : i1 loc(#loc124)
    %1355 = arith.ori %1351, %1354 : i1 loc(#loc124)
    %1356 = arith.andi %1341, %1355 : i1 loc(#loc124)
    %1357 = arith.ori %1351, %1353 : i1
    %1358 = arith.andi %1356, %1357 : i1
    %1359 = arith.andi %1320, %1354 : i1
    %1360 = arith.andi %1350, %1345 : i1
    %1361 = arith.andi %1322, %1360 : i1
    %1362 = arith.andi %1320, %1352 : i1
    %1363 = arith.ori %1361, %1362 : i1
    %1364 = arith.select %1322, %1331, %false : i1
    %1365 = arith.andi %1350, %1348 : i1
    %1366 = arith.andi %1322, %1365 : i1
    %1367 = arith.andi %1352, %1359 : i1
    %1368 = arith.andi %1320, %1367 : i1
    %1369 = arith.ori %1366, %1368 : i1
    %1370 = arith.select %1322, %1349, %value_80 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1371 = arith.ori %1314, %1315 : i1
    %1372 = arith.xori %1311, %true : i1 loc(#loc126)
    %1373 = arith.ori %1372, %1308 : i1
    acir.path %1311 {branch = 37 : i64, kind = "branch", txn = 6 : i64} loc(#loc126)
    acir.path %1311 {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc126)
    %1374 = arith.ori %1372, %1373 : i1 loc(#loc126)
    %value_82, %ready_83 = acir.read(%1311) {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1375 = arith.andi %1374, %ready_83 : i1 loc(#loc124)
    %1376 = arith.ori %1372, %1375 : i1 loc(#loc124)
    %1377 = arith.andi %1358, %1376 : i1 loc(#loc124)
    %1378 = arith.ori %1372, %1374 : i1
    %1379 = arith.andi %1377, %1378 : i1
    %1380 = arith.andi %1311, %1375 : i1
    %1381 = arith.andi %1371, %1363 : i1
    %1382 = arith.andi %1313, %1381 : i1
    %1383 = arith.andi %1311, %1373 : i1
    %1384 = arith.ori %1382, %1383 : i1
    %1385 = arith.select %1313, %1364, %false : i1
    %1386 = arith.andi %1371, %1369 : i1
    %1387 = arith.andi %1313, %1386 : i1
    %1388 = arith.andi %1373, %1380 : i1
    %1389 = arith.andi %1311, %1388 : i1
    %1390 = arith.ori %1387, %1389 : i1
    %1391 = arith.select %1313, %1370, %value_82 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1392 = arith.ori %1299, %1301 : i1
    %1393 = arith.andi %1392, %1384 : i1
    %1394 = arith.ori %1299, %1393 : i1
    %1395 = arith.andi %1379, %1394 : i1
    %1396 = arith.andi %1294, %1385 : i1
    %1397 = arith.xori %1385, %true : i1
    %1398 = arith.andi %1294, %1397 : i1
    %1399 = arith.xori %1398, %true : i1
    %1400 = arith.ori %1399, %1393 : i1
    %1401 = arith.xori %1396, %true : i1 loc(#loc127)
    %1402 = arith.ori %1401, %1393 : i1
    acir.path %1396 {branch = 38 : i64, kind = "branch", txn = 6 : i64} loc(#loc127)
    acir.path %1396 {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc127)
    %1403 = arith.ori %1401, %1402 : i1 loc(#loc127)
    %value_84, %ready_85 = acir.read(%1396) {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1404 = arith.andi %1403, %ready_85 : i1 loc(#loc124)
    %1405 = arith.ori %1401, %1404 : i1 loc(#loc124)
    %1406 = arith.andi %1395, %1405 : i1 loc(#loc124)
    %1407 = arith.ori %1401, %1403 : i1
    %1408 = arith.andi %1406, %1407 : i1
    %1409 = arith.andi %1396, %1404 : i1
    %1410 = arith.andi %1400, %1384 : i1
    %1411 = arith.andi %1398, %1410 : i1
    %1412 = arith.andi %1396, %1402 : i1
    %1413 = arith.ori %1411, %1412 : i1
    %1414 = arith.select %1398, %1385, %false : i1
    %1415 = arith.andi %1400, %1390 : i1
    %1416 = arith.andi %1398, %1415 : i1
    %1417 = arith.andi %1402, %1409 : i1
    %1418 = arith.andi %1396, %1417 : i1
    %1419 = arith.ori %1416, %1418 : i1
    %1420 = arith.select %1398, %1391, %value_84 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1421 = arith.ori %1299, %1392 : i1
    %1422 = acir.unary %1414 "not" {acir.source_type = "bool"} : i1 -> i1
    %1423 = arith.andi %1421, %1413 : i1
    %1424 = arith.ori %1299, %1423 : i1
    %1425 = arith.andi %1408, %1424 : i1
    %1426 = arith.andi %1421, %1419 : i1
    %1427 = arith.andi %1294, %1426 : i1
    %1428 = acir.extract %1420 {acir.id = "v45", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc128)
    %1429 = arith.andi %1300, %1427 : i1
    %1430 = arith.ori %1299, %1429 : i1
    %1431 = arith.andi %1425, %1430 : i1
    %1432 = arith.andi %1294, %1428 : i1
    %1433 = arith.xori %1428, %true : i1
    %1434 = arith.andi %1294, %1433 : i1
    %1435 = arith.xori %1434, %true : i1
    %1436 = arith.ori %1435, %1429 : i1
    %1437 = arith.xori %1432, %true : i1
    %1438 = arith.ori %1437, %1429 : i1
    %1439 = acir.extract %1420 {acir.id = "v46", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc129)
    %1440 = arith.andi %1434, %1436 : i1
    %1441 = arith.andi %1438, %1427 : i1
    %1442 = arith.andi %1432, %1441 : i1
    %1443 = arith.ori %1440, %1442 : i1
    %1444 = arith.select %1434, %false, %1439 : i1
    %1445 = arith.andi %1301, %1443 : i1
    %1446 = arith.ori %1299, %1445 : i1
    %1447 = arith.andi %1431, %1446 : i1
    %1448 = arith.andi %1294, %1444 : i1
    %1449 = arith.xori %1444, %true : i1
    %1450 = arith.andi %1294, %1449 : i1
    %1451 = arith.xori %1450, %true : i1
    %1452 = arith.ori %1451, %1445 : i1
    %1453 = arith.xori %1448, %true : i1
    %1454 = arith.ori %1453, %1445 : i1
    %1455 = acir.extract %1420 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc130)
    %1456 = arith.andi %1427, %1253 : i1 loc(#loc130)
    %1457 = arith.cmpi eq, %1455, %1254 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc130)
    %1458 = arith.andi %1450, %1452 : i1
    %1459 = arith.andi %1454, %1456 : i1
    %1460 = arith.andi %1448, %1459 : i1
    %1461 = arith.ori %1458, %1460 : i1
    %1462 = arith.select %1450, %false, %1457 : i1
    %1463 = arith.andi %1392, %1461 : i1
    %1464 = arith.ori %1299, %1463 : i1
    %1465 = arith.andi %1447, %1464 : i1
    %1466 = arith.andi %1294, %1462 : i1
    %1467 = arith.xori %1462, %true : i1
    %1468 = arith.andi %1294, %1467 : i1
    %1469 = arith.xori %1468, %true : i1
    %1470 = arith.ori %1469, %1463 : i1
    %1471 = arith.xori %1466, %true : i1
    %1472 = arith.ori %1471, %1463 : i1
    %1473 = acir.extract %1420 {acir.id = "v58", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc131)
    %1474 = acir.aggregate(%true, %1254, %1473) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc132)
    %1475 = arith.andi %1470, %1284 : i1
    %1476 = arith.andi %1468, %1475 : i1
    %1477 = arith.andi %1466, %1472 : i1
    %1478 = arith.ori %1476, %1477 : i1
    %1479 = arith.select %1468, %1285, %false : i1
    %1480 = arith.andi %1470, %1288 : i1
    %1481 = arith.andi %1468, %1480 : i1
    %1482 = arith.andi %1472, %1456 : i1
    %1483 = arith.andi %1466, %1482 : i1
    %1484 = arith.ori %1481, %1483 : i1
    %1485 = arith.select %1468, %1289, %1474 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1486 = arith.andi %1421, %1478 : i1
    %1487 = arith.ori %1299, %1486 : i1
    %1488 = arith.andi %1465, %1487 : i1
    %1489 = arith.andi %1294, %1479 : i1
    %1490 = arith.xori %1479, %true : i1
    %1491 = arith.andi %1294, %1490 : i1
    %1492 = arith.xori %1491, %true : i1
    %1493 = arith.ori %1492, %1486 : i1
    %1494 = arith.xori %1489, %true : i1
    %1495 = arith.ori %1494, %1486 : i1
    %1496 = acir.aggregate(%false, %1254, %c0_i32) {acir.id = "v70", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc133)
    %1497 = arith.andi %1493, %1478 : i1
    %1498 = arith.andi %1491, %1497 : i1
    %1499 = arith.andi %1489, %1495 : i1
    %1500 = arith.ori %1498, %1499 : i1
    %1501 = arith.select %1491, %1479, %false : i1
    %1502 = arith.andi %1493, %1484 : i1
    %1503 = arith.andi %1491, %1502 : i1
    %1504 = arith.andi %1495, %1253 : i1
    %1505 = arith.andi %1489, %1504 : i1
    %1506 = arith.ori %1503, %1505 : i1
    %1507 = arith.select %1491, %1485, %1496 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1508 = arith.ori %1299, %1421 : i1
    %1509 = arith.andi %1298, %1284 : i1
    %1510 = arith.andi %1296, %1509 : i1
    %1511 = arith.andi %1508, %1500 : i1
    %1512 = arith.andi %1294, %1511 : i1
    %1513 = arith.ori %1510, %1512 : i1
    %1514 = arith.select %1296, %1285, %1501 : i1
    %1515 = arith.andi %1298, %1288 : i1
    %1516 = arith.andi %1296, %1515 : i1
    %1517 = arith.andi %1508, %1506 : i1
    %1518 = arith.andi %1294, %1517 : i1
    %1519 = arith.ori %1516, %1518 : i1
    %1520 = arith.select %1296, %1289, %1507 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1521 = arith.ori %1229, %1290 : i1
    %1522 = arith.andi %1244, %1236 : i1
    %1523 = arith.andi %1242, %1522 : i1
    %1524 = arith.andi %1521, %1513 : i1
    %1525 = arith.andi %1228, %1524 : i1
    %1526 = arith.ori %1523, %1525 : i1
    %1527 = arith.select %1242, %1228, %1514 : i1
    %1528 = arith.andi %1521, %1519 : i1
    %1529 = arith.andi %1228, %1528 : i1
    %1530 = arith.ori %1523, %1529 : i1
    %1531 = arith.select %1242, %1237, %1520 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1532 = arith.ori %1201, %1238 : i1
    %1533 = acir.unary %1527 "not" {acir.source_type = "bool"} : i1 -> i1
    %1534 = arith.andi %1532, %1526 : i1
    %1535 = arith.ori %1201, %1534 : i1
    %1536 = arith.andi %1488, %1535 : i1
    %1537 = arith.andi %1532, %1530 : i1
    %1538 = arith.andi %1200, %1537 : i1
    %1539 = acir.extract %value_68 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc134)
    acir.path %1200 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc135)
    %1540 = arith.cmpi ult, %1539, %c8_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc108)
    %1541 = arith.andi %1536, %1222 : i1 loc(#loc109)
    %1542 = arith.cmpi eq, %1539, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc110)
    %1543 = arith.andi %1541, %1222 : i1
    %1544 = arith.andi %1200, %1542 : i1
    %1545 = arith.cmpi ne, %1539, %c0_i32 : i32 loc(#loc110)
    %1546 = arith.andi %1200, %1545 : i1
    %1547 = arith.xori %1546, %true : i1 loc(#loc111)
    %1548 = arith.ori %1547, %1221 : i1
    %1549 = arith.xori %1544, %true : i1
    %1550 = arith.ori %1549, %1221 : i1
    %1551 = arith.andi %1546, %1548 : i1
    %1552 = arith.andi %1544, %1550 : i1
    %1553 = arith.ori %1551, %1552 : i1
    %1554 = arith.select %1546, %1219, %1233 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1555 = arith.andi %1238, %1553 : i1
    %1556 = arith.ori %1201, %1555 : i1
    %1557 = arith.andi %1543, %1556 : i1
    %1558 = arith.andi %1200, %1547 : i1
    %1559 = arith.xori %1558, %true : i1
    %1560 = arith.ori %1559, %1555 : i1
    %1561 = arith.ori %1547, %1555 : i1
    acir.path %1546 {branch = 39 : i64, kind = "branch", txn = 6 : i64} loc(#loc111)
    %value_86, %ready_87 = acir.read(%1546) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc111)
    %1562 = arith.andi %1561, %ready_87 : i1 loc(#loc111)
    %1563 = arith.ori %1547, %1562 : i1 loc(#loc111)
    %1564 = arith.andi %1557, %1563 : i1 loc(#loc111)
    %1565 = arith.andi %ready_87, %1218 : i1 loc(#loc111)
    %1566 = arith.extui %1539 : i32 to i64 loc(#loc111)
    %1567 = arith.cmpi ult, %1566, %c8_i64 : i64 loc(#loc111)
    %1568 = arith.select %1567, %1566, %c0_i64 : i64 loc(#loc111)
    %1569 = arith.andi %1565, %1567 : i1 loc(#loc111)
    %1570 = "acir.array.index"(%value_86, %1568) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc111)
    %value_88, %ready_89 = acir.read(%1546) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc114)
    %1571 = arith.andi %1561, %ready_89 : i1 loc(#loc114)
    %1572 = arith.ori %1547, %1571 : i1 loc(#loc114)
    %1573 = arith.andi %1564, %1572 : i1 loc(#loc114)
    %1574 = arith.andi %ready_89, %1218 : i1 loc(#loc114)
    %1575 = arith.andi %1574, %1567 : i1 loc(#loc114)
    %1576 = "acir.array.index"(%value_88, %1568) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc114)
    %1577 = arith.andi %1569, %1575 : i1 loc(#loc115)
    %1578 = arith.cmpi eq, %1570, %1576 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc115)
    %1579 = arith.andi %1561, %1577 : i1
    %1580 = arith.ori %1547, %1579 : i1
    %1581 = arith.andi %1573, %1580 : i1
    %1582 = arith.andi %1546, %1578 : i1
    %1583 = arith.cmpi ne, %1570, %1576 : i32 loc(#loc115)
    %1584 = arith.andi %1546, %1583 : i1
    %1585 = arith.xori %1584, %true : i1
    %1586 = arith.ori %1585, %1579 : i1
    %1587 = arith.xori %1582, %true : i1 loc(#loc116)
    %1588 = arith.ori %1587, %1579 : i1
    acir.path %1582 {branch = 40 : i64, kind = "branch", txn = 6 : i64} loc(#loc116)
    %value_90, %ready_91 = acir.read(%1582) {branch = 40 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc116)
    %1589 = arith.andi %1588, %ready_91 : i1 loc(#loc116)
    %1590 = arith.ori %1587, %1589 : i1 loc(#loc116)
    %1591 = arith.andi %1581, %1590 : i1 loc(#loc116)
    %1592 = arith.andi %ready_91, %1218 : i1 loc(#loc116)
    %1593 = arith.andi %1592, %1567 : i1 loc(#loc116)
    %1594 = "acir.array.index"(%value_90, %1568) : (!acir.array<i32, [8]>, i64) -> i32 loc(#loc116)
    %1595 = arith.andi %1569, %1593 : i1 loc(#loc117)
    %1596 = acir.aggregate(%true, %1570, %1594) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc117)
    %1597 = arith.andi %1586, %1553 : i1
    %1598 = arith.andi %1584, %1597 : i1
    %1599 = arith.andi %1582, %1588 : i1
    %1600 = arith.ori %1598, %1599 : i1
    %1601 = arith.select %1584, %1546, %false : i1
    %1602 = arith.andi %1588, %1595 : i1
    %1603 = arith.andi %1582, %1602 : i1
    %1604 = arith.ori %1598, %1603 : i1
    %1605 = arith.select %1584, %1554, %1596 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1606 = arith.ori %1547, %1561 : i1
    %1607 = arith.andi %1606, %1600 : i1
    %1608 = arith.ori %1547, %1607 : i1
    %1609 = arith.andi %1591, %1608 : i1
    %1610 = arith.andi %1546, %1601 : i1
    %1611 = arith.xori %1601, %true : i1
    %1612 = arith.andi %1546, %1611 : i1
    %1613 = arith.xori %1612, %true : i1
    %1614 = arith.ori %1613, %1607 : i1
    %1615 = arith.xori %1610, %true : i1 loc(#loc118)
    %1616 = arith.ori %1615, %1607 : i1
    acir.path %1610 {branch = 41 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %1610 {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc118)
    %1617 = arith.ori %1615, %1616 : i1 loc(#loc118)
    %1618 = arith.andi %1616, %1569 : i1 loc(#loc118)
    %1619 = arith.andi %1610, %1618 : i1 loc(#loc118)
    %1620 = arith.subi %1570, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc119)
    %1621 = arith.andi %1620, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc119)
    %1622 = arith.cmpi eq, %1621, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc120)
    %1623 = arith.andi %1617, %1619 : i1
    %1624 = arith.ori %1615, %1623 : i1
    %1625 = arith.andi %1609, %1624 : i1
    %1626 = arith.andi %1610, %1622 : i1
    %1627 = arith.cmpi ne, %1621, %c0_i32 : i32 loc(#loc120)
    %1628 = arith.andi %1610, %1627 : i1
    %1629 = arith.xori %1628, %true : i1
    %1630 = arith.ori %1629, %1623 : i1
    %1631 = arith.cmpi eq, %1621, %c1_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc121)
    %1632 = arith.andi %1630, %1619 : i1
    %1633 = arith.ori %1629, %1632 : i1
    %1634 = arith.andi %1625, %1633 : i1
    %1635 = arith.andi %1628, %1631 : i1
    %1636 = arith.cmpi ne, %1621, %c1_i32 : i32 loc(#loc121)
    %1637 = arith.andi %1628, %1636 : i1
    %1638 = arith.xori %1637, %true : i1
    %1639 = arith.ori %1638, %1632 : i1
    %1640 = arith.cmpi eq, %1621, %c2_i32 {acir.id = "v22", acir.source_type = "bool"} : i32 loc(#loc122)
    %1641 = arith.andi %1639, %1619 : i1
    %1642 = arith.ori %1638, %1641 : i1
    %1643 = arith.andi %1634, %1642 : i1
    %1644 = arith.andi %1637, %1640 : i1
    %1645 = arith.cmpi ne, %1621, %c2_i32 : i32 loc(#loc122)
    %1646 = arith.andi %1637, %1645 : i1
    %1647 = arith.xori %1646, %true : i1
    %1648 = arith.ori %1647, %1641 : i1
    %1649 = arith.xori %1644, %true : i1 loc(#loc123)
    %1650 = arith.ori %1649, %1641 : i1
    acir.path %1644 {branch = 42 : i64, kind = "branch", txn = 6 : i64} loc(#loc123)
    acir.path %1644 {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc123)
    %1651 = arith.ori %1649, %1650 : i1 loc(#loc123)
    %value_92, %ready_93 = acir.read(%1644) {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1652 = arith.andi %1651, %ready_93 : i1 loc(#loc124)
    %1653 = arith.ori %1649, %1652 : i1 loc(#loc124)
    %1654 = arith.andi %1643, %1653 : i1 loc(#loc124)
    %1655 = arith.ori %1649, %1651 : i1
    %1656 = arith.andi %1654, %1655 : i1
    %1657 = arith.andi %1644, %1652 : i1
    %1658 = arith.andi %1646, %1648 : i1
    %1659 = arith.andi %1644, %1650 : i1
    %1660 = arith.ori %1658, %1659 : i1
    %1661 = arith.andi %1650, %1657 : i1
    %1662 = arith.andi %1644, %1661 : i1
    %1663 = arith.ori %1658, %1662 : i1
    %1664 = arith.select %1646, %1304, %value_92 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1665 = arith.ori %1638, %1639 : i1
    %1666 = arith.xori %1635, %true : i1 loc(#loc125)
    %1667 = arith.ori %1666, %1632 : i1
    acir.path %1635 {branch = 43 : i64, kind = "branch", txn = 6 : i64} loc(#loc125)
    acir.path %1635 {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc125)
    %1668 = arith.ori %1666, %1667 : i1 loc(#loc125)
    %value_94, %ready_95 = acir.read(%1635) {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1669 = arith.andi %1668, %ready_95 : i1 loc(#loc124)
    %1670 = arith.ori %1666, %1669 : i1 loc(#loc124)
    %1671 = arith.andi %1656, %1670 : i1 loc(#loc124)
    %1672 = arith.ori %1666, %1668 : i1
    %1673 = arith.andi %1671, %1672 : i1
    %1674 = arith.andi %1635, %1669 : i1
    %1675 = arith.andi %1665, %1660 : i1
    %1676 = arith.andi %1637, %1675 : i1
    %1677 = arith.andi %1635, %1667 : i1
    %1678 = arith.ori %1676, %1677 : i1
    %1679 = arith.select %1637, %1646, %false : i1
    %1680 = arith.andi %1665, %1663 : i1
    %1681 = arith.andi %1637, %1680 : i1
    %1682 = arith.andi %1667, %1674 : i1
    %1683 = arith.andi %1635, %1682 : i1
    %1684 = arith.ori %1681, %1683 : i1
    %1685 = arith.select %1637, %1664, %value_94 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1686 = arith.ori %1629, %1630 : i1
    %1687 = arith.xori %1626, %true : i1 loc(#loc126)
    %1688 = arith.ori %1687, %1623 : i1
    acir.path %1626 {branch = 44 : i64, kind = "branch", txn = 6 : i64} loc(#loc126)
    acir.path %1626 {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc126)
    %1689 = arith.ori %1687, %1688 : i1 loc(#loc126)
    %value_96, %ready_97 = acir.read(%1626) {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1690 = arith.andi %1689, %ready_97 : i1 loc(#loc124)
    %1691 = arith.ori %1687, %1690 : i1 loc(#loc124)
    %1692 = arith.andi %1673, %1691 : i1 loc(#loc124)
    %1693 = arith.ori %1687, %1689 : i1
    %1694 = arith.andi %1692, %1693 : i1
    %1695 = arith.andi %1626, %1690 : i1
    %1696 = arith.andi %1686, %1678 : i1
    %1697 = arith.andi %1628, %1696 : i1
    %1698 = arith.andi %1626, %1688 : i1
    %1699 = arith.ori %1697, %1698 : i1
    %1700 = arith.select %1628, %1679, %false : i1
    %1701 = arith.andi %1686, %1684 : i1
    %1702 = arith.andi %1628, %1701 : i1
    %1703 = arith.andi %1688, %1695 : i1
    %1704 = arith.andi %1626, %1703 : i1
    %1705 = arith.ori %1702, %1704 : i1
    %1706 = arith.select %1628, %1685, %value_96 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1707 = arith.ori %1615, %1617 : i1
    %1708 = arith.andi %1707, %1699 : i1
    %1709 = arith.ori %1615, %1708 : i1
    %1710 = arith.andi %1694, %1709 : i1
    %1711 = arith.andi %1610, %1700 : i1
    %1712 = arith.xori %1700, %true : i1
    %1713 = arith.andi %1610, %1712 : i1
    %1714 = arith.xori %1713, %true : i1
    %1715 = arith.ori %1714, %1708 : i1
    %1716 = arith.xori %1711, %true : i1 loc(#loc127)
    %1717 = arith.ori %1716, %1708 : i1
    acir.path %1711 {branch = 45 : i64, kind = "branch", txn = 6 : i64} loc(#loc127)
    acir.path %1711 {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc127)
    %1718 = arith.ori %1716, %1717 : i1 loc(#loc127)
    %value_98, %ready_99 = acir.read(%1711) {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc124)
    %1719 = arith.andi %1718, %ready_99 : i1 loc(#loc124)
    %1720 = arith.ori %1716, %1719 : i1 loc(#loc124)
    %1721 = arith.andi %1710, %1720 : i1 loc(#loc124)
    %1722 = arith.ori %1716, %1718 : i1
    %1723 = arith.andi %1721, %1722 : i1
    %1724 = arith.andi %1711, %1719 : i1
    %1725 = arith.andi %1715, %1699 : i1
    %1726 = arith.andi %1713, %1725 : i1
    %1727 = arith.andi %1711, %1717 : i1
    %1728 = arith.ori %1726, %1727 : i1
    %1729 = arith.select %1713, %1700, %false : i1
    %1730 = arith.andi %1715, %1705 : i1
    %1731 = arith.andi %1713, %1730 : i1
    %1732 = arith.andi %1717, %1724 : i1
    %1733 = arith.andi %1711, %1732 : i1
    %1734 = arith.ori %1731, %1733 : i1
    %1735 = arith.select %1713, %1706, %value_98 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %1736 = arith.ori %1615, %1707 : i1
    %1737 = acir.unary %1729 "not" {acir.source_type = "bool"} : i1 -> i1
    %1738 = arith.andi %1736, %1728 : i1
    %1739 = arith.ori %1615, %1738 : i1
    %1740 = arith.andi %1723, %1739 : i1
    %1741 = arith.andi %1736, %1734 : i1
    %1742 = arith.andi %1610, %1741 : i1
    %1743 = acir.extract %1735 {acir.id = "v45", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc128)
    %1744 = arith.andi %1616, %1742 : i1
    %1745 = arith.ori %1615, %1744 : i1
    %1746 = arith.andi %1740, %1745 : i1
    %1747 = arith.andi %1610, %1743 : i1
    %1748 = arith.xori %1743, %true : i1
    %1749 = arith.andi %1610, %1748 : i1
    %1750 = arith.xori %1749, %true : i1
    %1751 = arith.ori %1750, %1744 : i1
    %1752 = arith.xori %1747, %true : i1
    %1753 = arith.ori %1752, %1744 : i1
    %1754 = acir.extract %1735 {acir.id = "v46", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc129)
    %1755 = arith.andi %1749, %1751 : i1
    %1756 = arith.andi %1753, %1742 : i1
    %1757 = arith.andi %1747, %1756 : i1
    %1758 = arith.ori %1755, %1757 : i1
    %1759 = arith.select %1749, %false, %1754 : i1
    %1760 = arith.andi %1617, %1758 : i1
    %1761 = arith.ori %1615, %1760 : i1
    %1762 = arith.andi %1746, %1761 : i1
    %1763 = arith.andi %1610, %1759 : i1
    %1764 = arith.xori %1759, %true : i1
    %1765 = arith.andi %1610, %1764 : i1
    %1766 = arith.xori %1765, %true : i1
    %1767 = arith.ori %1766, %1760 : i1
    %1768 = arith.xori %1763, %true : i1
    %1769 = arith.ori %1768, %1760 : i1
    %1770 = acir.extract %1735 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc130)
    %1771 = arith.andi %1742, %1569 : i1 loc(#loc130)
    %1772 = arith.cmpi eq, %1770, %1570 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc130)
    %1773 = arith.andi %1765, %1767 : i1
    %1774 = arith.andi %1769, %1771 : i1
    %1775 = arith.andi %1763, %1774 : i1
    %1776 = arith.ori %1773, %1775 : i1
    %1777 = arith.select %1765, %false, %1772 : i1
    %1778 = arith.andi %1707, %1776 : i1
    %1779 = arith.ori %1615, %1778 : i1
    %1780 = arith.andi %1762, %1779 : i1
    %1781 = arith.andi %1610, %1777 : i1
    %1782 = arith.xori %1777, %true : i1
    %1783 = arith.andi %1610, %1782 : i1
    %1784 = arith.xori %1783, %true : i1
    %1785 = arith.ori %1784, %1778 : i1
    %1786 = arith.xori %1781, %true : i1
    %1787 = arith.ori %1786, %1778 : i1
    %1788 = acir.extract %1735 {acir.id = "v58", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc131)
    %1789 = acir.aggregate(%true, %1570, %1788) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc132)
    %1790 = arith.andi %1785, %1600 : i1
    %1791 = arith.andi %1783, %1790 : i1
    %1792 = arith.andi %1781, %1787 : i1
    %1793 = arith.ori %1791, %1792 : i1
    %1794 = arith.select %1783, %1601, %false : i1
    %1795 = arith.andi %1785, %1604 : i1
    %1796 = arith.andi %1783, %1795 : i1
    %1797 = arith.andi %1787, %1771 : i1
    %1798 = arith.andi %1781, %1797 : i1
    %1799 = arith.ori %1796, %1798 : i1
    %1800 = arith.select %1783, %1605, %1789 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1801 = arith.andi %1736, %1793 : i1
    %1802 = arith.ori %1615, %1801 : i1
    %1803 = arith.andi %1780, %1802 : i1
    %1804 = arith.andi %1610, %1794 : i1
    %1805 = arith.xori %1794, %true : i1
    %1806 = arith.andi %1610, %1805 : i1
    %1807 = arith.xori %1806, %true : i1
    %1808 = arith.ori %1807, %1801 : i1
    %1809 = arith.xori %1804, %true : i1
    %1810 = arith.ori %1809, %1801 : i1
    %1811 = acir.aggregate(%false, %1570, %c0_i32) {acir.id = "v70", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc133)
    %1812 = arith.andi %1808, %1793 : i1
    %1813 = arith.andi %1806, %1812 : i1
    %1814 = arith.andi %1804, %1810 : i1
    %1815 = arith.ori %1813, %1814 : i1
    %1816 = arith.select %1806, %1794, %false : i1
    %1817 = arith.andi %1808, %1799 : i1
    %1818 = arith.andi %1806, %1817 : i1
    %1819 = arith.andi %1810, %1569 : i1
    %1820 = arith.andi %1804, %1819 : i1
    %1821 = arith.ori %1818, %1820 : i1
    %1822 = arith.select %1806, %1800, %1811 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1823 = arith.ori %1615, %1736 : i1
    %1824 = arith.andi %1614, %1600 : i1
    %1825 = arith.andi %1612, %1824 : i1
    %1826 = arith.andi %1823, %1815 : i1
    %1827 = arith.andi %1610, %1826 : i1
    %1828 = arith.ori %1825, %1827 : i1
    %1829 = arith.select %1612, %1601, %1816 : i1
    %1830 = arith.andi %1614, %1604 : i1
    %1831 = arith.andi %1612, %1830 : i1
    %1832 = arith.andi %1823, %1821 : i1
    %1833 = arith.andi %1610, %1832 : i1
    %1834 = arith.ori %1831, %1833 : i1
    %1835 = arith.select %1612, %1605, %1822 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1836 = arith.ori %1547, %1606 : i1
    %1837 = arith.andi %1560, %1553 : i1
    %1838 = arith.andi %1558, %1837 : i1
    %1839 = arith.andi %1836, %1828 : i1
    %1840 = arith.andi %1546, %1839 : i1
    %1841 = arith.ori %1838, %1840 : i1
    %1842 = arith.select %1558, %1546, %1829 : i1
    %1843 = arith.andi %1836, %1834 : i1
    %1844 = arith.andi %1546, %1843 : i1
    %1845 = arith.ori %1838, %1844 : i1
    %1846 = arith.select %1558, %1554, %1835 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1847 = acir.unary %1842 "not" {acir.source_type = "bool"} : i1 -> i1
    %1848 = arith.andi %1532, %1841 : i1
    %1849 = arith.ori %1201, %1848 : i1
    %1850 = arith.andi %1803, %1849 : i1
    %1851 = arith.andi %1532, %1845 : i1
    %1852 = arith.andi %1200, %1851 : i1
    %1853 = acir.extract %value_68 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc136)
    acir.path %1200 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc113)
    acir.path %1200 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc113)
    %value_100, %ready_101 = acir.read(%1200) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc137)
    %1854 = arith.andi %1238, %ready_101 : i1 loc(#loc137)
    %1855 = arith.ori %1201, %1854 : i1 loc(#loc137)
    %1856 = arith.andi %1850, %1855 : i1 loc(#loc137)
    %value_102, %ready_103 = acir.read(%1200) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc138)
    %1857 = arith.andi %1238, %ready_103 : i1 loc(#loc138)
    %1858 = arith.ori %1201, %1857 : i1 loc(#loc138)
    %1859 = arith.andi %1856, %1858 : i1 loc(#loc138)
    %1860 = arith.andi %ready_101, %ready_103 : i1 loc(#loc137)
    %1861 = arith.subi %value_100, %value_102 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc137)
    %1862 = arith.cmpi ult, %1861, %c4_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc137)
    %1863 = arith.andi %1859, %1532 : i1
    %1864 = arith.andi %1238, %1860 : i1
    %1865 = arith.andi %1200, %1864 : i1
    %1866 = arith.andi %1217, %1865 : i1 loc(#loc139)
    %1867 = arith.andi %1200, %1866 : i1 loc(#loc139)
    %1868 = arith.andi %1867, %1862 : i1 loc(#loc139)
    %1869 = arith.andi %1204, %1868 : i1 loc(#loc139)
    %1870 = arith.ori %1201, %1869 : i1 loc(#loc139)
    %1871 = arith.andi %1863, %1870 : i1 loc(#loc139)
    %1872 = arith.andi %1869, %ready_69 : i1 loc(#loc139)
    %1873 = arith.andi %1200, %1872 : i1 loc(#loc139)
    %value_104, %ready_105 = acir.read(%1200) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc140)
    %1874 = arith.andi %1870, %ready_105 : i1 loc(#loc140)
    %1875 = arith.ori %1201, %1874 : i1 loc(#loc140)
    %1876 = arith.andi %1871, %1875 : i1 loc(#loc140)
    %1877 = arith.subi %value_104, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc141)
    %1878 = arith.andi %1877, %c3_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc141)
    %1879 = arith.cmpi eq, %1878, %c0_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc142)
    %1880 = arith.andi %1876, %1875 : i1
    %1881 = arith.andi %1200, %1879 : i1
    %1882 = arith.cmpi ne, %1878, %c0_i32 : i32 loc(#loc142)
    %1883 = arith.andi %1200, %1882 : i1
    %1884 = arith.xori %1883, %true : i1
    %1885 = arith.ori %1884, %1874 : i1
    %1886 = arith.cmpi eq, %1878, %c1_i32 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc143)
    %1887 = arith.andi %1885, %ready_105 : i1
    %1888 = arith.ori %1884, %1887 : i1
    %1889 = arith.andi %1880, %1888 : i1
    %1890 = arith.andi %1883, %1886 : i1
    %1891 = arith.cmpi ne, %1878, %c1_i32 : i32 loc(#loc143)
    %1892 = arith.andi %1883, %1891 : i1
    %1893 = arith.xori %1892, %true : i1
    %1894 = arith.ori %1893, %1887 : i1
    %1895 = arith.cmpi eq, %1878, %c2_i32 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc144)
    %1896 = arith.andi %1894, %ready_105 : i1
    %1897 = arith.ori %1893, %1896 : i1
    %1898 = arith.andi %1889, %1897 : i1
    %1899 = arith.andi %1892, %1895 : i1
    %1900 = arith.cmpi ne, %1878, %c2_i32 : i32 loc(#loc144)
    %1901 = arith.andi %1892, %1900 : i1
    %1902 = arith.xori %1901, %true : i1 loc(#loc145)
    %1903 = arith.ori %1902, %1896 : i1
    acir.path %1901 {branch = 46 : i64, kind = "branch", txn = 6 : i64} loc(#loc145)
    acir.path %1901 {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc145)
    %1904 = arith.ori %1902, %1903 : i1 loc(#loc145)
    acir.path %1901 {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate", "top.rob.slot3:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc145)
    %1905 = arith.ori %1902, %1904 : i1 loc(#loc145)
    %value_106, %ready_107 = acir.read(%1901) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate", "top.rob.slot3:free"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc146)
    %1906 = arith.andi %1905, %ready_107 : i1 loc(#loc146)
    %1907 = arith.ori %1902, %1906 : i1 loc(#loc146)
    %1908 = arith.andi %1898, %1907 : i1 loc(#loc146)
    %1909 = acir.extract %value_106 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc146)
    %1910 = acir.unary %1909 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc147)
    %1911 = arith.ori %1902, %1905 : i1
    %1912 = arith.andi %1908, %1911 : i1
    %1913 = arith.andi %1901, %1906 : i1
    %1914 = arith.andi %1904, %1913 : i1 loc(#loc148)
    %1915 = arith.andi %1901, %1914 : i1 loc(#loc148)
    %1916 = arith.andi %1915, %1910 : i1 loc(#loc148)
    %1917 = arith.andi %1903, %1916 : i1 loc(#loc148)
    %1918 = arith.ori %1902, %1917 : i1 loc(#loc148)
    %1919 = arith.andi %1912, %1918 : i1 loc(#loc148)
    %1920 = arith.andi %1917, %ready_105 : i1 loc(#loc148)
    %1921 = arith.andi %1901, %1920 : i1 loc(#loc148)
    %1922 = arith.andi %1917, %1873 : i1 loc(#loc148)
    %1923 = arith.andi %1901, %1922 : i1 loc(#loc148)
    %1924 = arith.andi %1921, %1923 : i1 loc(#loc149)
    %1925 = acir.aggregate(%true, %false, %value_104, %1853, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc149)
    %1926 = arith.andi %1918, %1924 : i1 loc(#loc150)
    %1927 = arith.ori %1902, %1926 : i1 loc(#loc150)
    %1928 = arith.andi %1919, %1927 : i1 loc(#loc150)
    %1929 = arith.xori %1899, %true : i1 loc(#loc151)
    %1930 = arith.ori %1929, %1896 : i1
    acir.path %1899 {branch = 47 : i64, kind = "branch", txn = 6 : i64} loc(#loc151)
    acir.path %1899 {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %1931 = arith.ori %1929, %1930 : i1 loc(#loc151)
    acir.path %1899 {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate", "top.rob.slot2:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %1932 = arith.ori %1929, %1931 : i1 loc(#loc151)
    %value_108, %ready_109 = acir.read(%1899) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate", "top.rob.slot2:free"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc146)
    %1933 = arith.andi %1932, %ready_109 : i1 loc(#loc146)
    %1934 = arith.ori %1929, %1933 : i1 loc(#loc146)
    %1935 = arith.andi %1928, %1934 : i1 loc(#loc146)
    %1936 = acir.extract %value_108 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc146)
    %1937 = acir.unary %1936 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc147)
    %1938 = arith.ori %1929, %1932 : i1
    %1939 = arith.andi %1935, %1938 : i1
    %1940 = arith.andi %1899, %1933 : i1
    %1941 = arith.andi %1931, %1940 : i1 loc(#loc148)
    %1942 = arith.andi %1899, %1941 : i1 loc(#loc148)
    %1943 = arith.andi %1942, %1937 : i1 loc(#loc148)
    %1944 = arith.andi %1930, %1943 : i1 loc(#loc148)
    %1945 = arith.ori %1929, %1944 : i1 loc(#loc148)
    %1946 = arith.andi %1939, %1945 : i1 loc(#loc148)
    %1947 = arith.andi %1944, %ready_105 : i1 loc(#loc148)
    %1948 = arith.andi %1899, %1947 : i1 loc(#loc148)
    %1949 = arith.andi %1944, %1873 : i1 loc(#loc148)
    %1950 = arith.andi %1899, %1949 : i1 loc(#loc148)
    %1951 = arith.andi %1948, %1950 : i1 loc(#loc149)
    %1952 = arith.andi %1945, %1951 : i1 loc(#loc150)
    %1953 = arith.ori %1929, %1952 : i1 loc(#loc150)
    %1954 = arith.andi %1946, %1953 : i1 loc(#loc150)
    %1955 = arith.xori %1890, %true : i1 loc(#loc152)
    %1956 = arith.ori %1955, %1887 : i1
    acir.path %1890 {branch = 48 : i64, kind = "branch", txn = 6 : i64} loc(#loc152)
    acir.path %1890 {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc152)
    %1957 = arith.ori %1955, %1956 : i1 loc(#loc152)
    acir.path %1890 {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate", "top.rob.slot1:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc152)
    %1958 = arith.ori %1955, %1957 : i1 loc(#loc152)
    %value_110, %ready_111 = acir.read(%1890) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate", "top.rob.slot1:free"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc146)
    %1959 = arith.andi %1958, %ready_111 : i1 loc(#loc146)
    %1960 = arith.ori %1955, %1959 : i1 loc(#loc146)
    %1961 = arith.andi %1954, %1960 : i1 loc(#loc146)
    %1962 = acir.extract %value_110 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc146)
    %1963 = acir.unary %1962 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc147)
    %1964 = arith.ori %1955, %1958 : i1
    %1965 = arith.andi %1961, %1964 : i1
    %1966 = arith.andi %1890, %1959 : i1
    %1967 = arith.andi %1957, %1966 : i1 loc(#loc148)
    %1968 = arith.andi %1890, %1967 : i1 loc(#loc148)
    %1969 = arith.andi %1968, %1963 : i1 loc(#loc148)
    %1970 = arith.andi %1956, %1969 : i1 loc(#loc148)
    %1971 = arith.ori %1955, %1970 : i1 loc(#loc148)
    %1972 = arith.andi %1965, %1971 : i1 loc(#loc148)
    %1973 = arith.andi %1970, %ready_105 : i1 loc(#loc148)
    %1974 = arith.andi %1890, %1973 : i1 loc(#loc148)
    %1975 = arith.andi %1970, %1873 : i1 loc(#loc148)
    %1976 = arith.andi %1890, %1975 : i1 loc(#loc148)
    %1977 = arith.andi %1974, %1976 : i1 loc(#loc149)
    %1978 = arith.andi %1971, %1977 : i1 loc(#loc150)
    %1979 = arith.ori %1955, %1978 : i1 loc(#loc150)
    %1980 = arith.andi %1972, %1979 : i1 loc(#loc150)
    %1981 = arith.xori %1881, %true : i1 loc(#loc153)
    %1982 = arith.ori %1981, %1874 : i1
    acir.path %1881 {branch = 49 : i64, kind = "branch", txn = 6 : i64} loc(#loc153)
    acir.path %1881 {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc153)
    %1983 = arith.ori %1981, %1982 : i1 loc(#loc153)
    acir.path %1881 {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate", "top.rob.slot0:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc153)
    %1984 = arith.ori %1981, %1983 : i1 loc(#loc153)
    %value_112, %ready_113 = acir.read(%1881) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate", "top.rob.slot0:free"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc146)
    %1985 = arith.andi %1984, %ready_113 : i1 loc(#loc146)
    %1986 = arith.ori %1981, %1985 : i1 loc(#loc146)
    %1987 = arith.andi %1980, %1986 : i1 loc(#loc146)
    %1988 = acir.extract %value_112 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc146)
    %1989 = acir.unary %1988 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc147)
    %1990 = arith.ori %1981, %1984 : i1
    %1991 = arith.andi %1987, %1990 : i1
    %1992 = arith.andi %1881, %1985 : i1
    %1993 = arith.andi %1983, %1992 : i1 loc(#loc148)
    %1994 = arith.andi %1881, %1993 : i1 loc(#loc148)
    %1995 = arith.andi %1994, %1989 : i1 loc(#loc148)
    %1996 = arith.andi %1982, %1995 : i1 loc(#loc148)
    %1997 = arith.ori %1981, %1996 : i1 loc(#loc148)
    %1998 = arith.andi %1991, %1997 : i1 loc(#loc148)
    %1999 = arith.andi %1996, %ready_105 : i1 loc(#loc148)
    %2000 = arith.andi %1881, %1999 : i1 loc(#loc148)
    %2001 = arith.andi %1996, %1873 : i1 loc(#loc148)
    %2002 = arith.andi %1881, %2001 : i1 loc(#loc148)
    %2003 = arith.andi %2000, %2002 : i1 loc(#loc149)
    %2004 = arith.andi %1997, %2003 : i1 loc(#loc150)
    %2005 = arith.ori %1981, %2004 : i1 loc(#loc150)
    %2006 = arith.andi %1998, %2005 : i1 loc(#loc150)
    %2007 = arith.addi %value_104, %c1_i32 {acir.id = "v34", acir.source_type = "u32"} : i32 loc(#loc154)
    %2008 = arith.andi %2006, %1875 : i1 loc(#loc155)
    %2009 = arith.ori %1201, %1870 : i1
    %2010 = arith.andi %2008, %2009 : i1
    %2011 = arith.andi %1200, %1874 : i1
    %2012 = arith.andi %2011, %1868 : i1
    %2013 = acir.extract %value_68 {acir.id = "v12", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc156)
    acir.path %1200 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc157)
    %2014 = arith.andi %1204, %2012 : i1 loc(#loc157)
    %2015 = arith.andi %1200, %2014 : i1 loc(#loc157)
    %2016 = arith.cmpi ult, %2013, %c8_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc158)
    %2017 = arith.andi %2010, %1222 : i1 loc(#loc159)
    %2018 = arith.cmpi ne, %2013, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc160)
    %2019 = arith.andi %2017, %1222 : i1
    %2020 = arith.andi %1200, %2018 : i1
    %2021 = arith.xori %2020, %true : i1 loc(#loc161)
    %2022 = arith.ori %2021, %1221 : i1
    acir.path %2020 {branch = 50 : i64, kind = "branch", txn = 6 : i64} loc(#loc161)
    %value_114, %ready_115 = acir.read(%2020) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc161)
    %2023 = arith.andi %2022, %ready_115 : i1 loc(#loc161)
    %2024 = arith.ori %2021, %2023 : i1 loc(#loc161)
    %2025 = arith.andi %2019, %2024 : i1 loc(#loc161)
    %2026 = arith.andi %ready_115, %2015 : i1 loc(#loc161)
    %2027 = arith.andi %2026, %1218 : i1 loc(#loc161)
    %2028 = arith.extui %2013 : i32 to i64 loc(#loc161)
    %2029 = arith.cmpi ult, %2028, %c8_i64 : i64 loc(#loc161)
    %2030 = arith.andi %2027, %2029 : i1 loc(#loc161)
    %2031 = arith.select %2029, %2028, %c0_i64 : i64 loc(#loc161)
    %2032 = acir.update %value_114, %value_104[%2031] {acir.id = "v15", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i64) -> !acir.array<i32, [8]> loc(#loc161)
    %2033 = arith.andi %2022, %2030 : i1 loc(#loc161)
    %2034 = arith.ori %2021, %2033 : i1 loc(#loc161)
    %2035 = arith.andi %2025, %2034 : i1 loc(#loc161)
    %2036 = acir.extract %value_68 {acir.id = "v15", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc162)
    %2037 = acir.extract %value_68 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc163)
    %2038 = arith.andi %ready_69, %2012 : i1 loc(#loc164)
    %2039 = arith.andi %2038, %1538 : i1 loc(#loc164)
    %2040 = arith.andi %2039, %1852 : i1 loc(#loc164)
    %2041 = acir.aggregate(%true, %2036, %value_104, %2037, %1531, %1846) {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc164)
    acir.path %1200 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc165)
    %value_116, %ready_117 = acir.read(%1200) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    %2042 = arith.andi %1217, %ready_117 : i1 loc(#loc2)
    %2043 = arith.ori %1201, %2042 : i1 loc(#loc2)
    %2044 = arith.andi %2035, %2043 : i1 loc(#loc2)
    %2045 = arith.andi %2044, %1238 : i1
    %2046 = arith.andi %1200, %2042 : i1
    %2047 = acir.extract %value_116 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc165)
    %2048 = acir.unary %2047 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc166)
    %2049 = arith.andi %1204, %2046 : i1
    %2050 = arith.ori %1201, %2049 : i1
    %2051 = arith.andi %2045, %2050 : i1
    %2052 = arith.andi %1200, %2048 : i1
    %2053 = arith.xori %2048, %true : i1
    %2054 = arith.andi %1200, %2053 : i1
    %2055 = arith.xori %2054, %true : i1 loc(#loc167)
    %2056 = arith.ori %2055, %2049 : i1
    acir.path %2054 {branch = 51 : i64, kind = "branch", txn = 6 : i64} loc(#loc167)
    acir.path %2054 {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc167)
    %2057 = arith.ori %2055, %2056 : i1 loc(#loc167)
    acir.path %2054 {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc167)
    %2058 = arith.ori %2055, %2057 : i1 loc(#loc167)
    %value_118, %ready_119 = acir.read(%2054) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc168)
    %2059 = arith.andi %2058, %ready_119 : i1 loc(#loc168)
    %2060 = arith.ori %2055, %2059 : i1 loc(#loc168)
    %2061 = arith.andi %2051, %2060 : i1 loc(#loc168)
    %2062 = acir.extract %value_118 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc168)
    %2063 = acir.unary %2062 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc169)
    %2064 = arith.ori %2055, %2058 : i1
    %2065 = arith.andi %2061, %2064 : i1
    %2066 = arith.andi %2054, %2059 : i1
    %2067 = arith.andi %2057, %2066 : i1 loc(#loc170)
    %2068 = arith.andi %2054, %2067 : i1 loc(#loc170)
    %2069 = arith.andi %2068, %2063 : i1 loc(#loc170)
    %2070 = arith.andi %2056, %2069 : i1 loc(#loc170)
    %2071 = arith.ori %2055, %2070 : i1 loc(#loc170)
    %2072 = arith.andi %2065, %2071 : i1 loc(#loc170)
    %2073 = arith.andi %2070, %2040 : i1 loc(#loc170)
    %2074 = arith.andi %2054, %2073 : i1 loc(#loc170)
    %2075 = arith.andi %2071, %2074 : i1 loc(#loc171)
    %2076 = arith.ori %2055, %2075 : i1 loc(#loc171)
    %2077 = arith.andi %2072, %2076 : i1 loc(#loc171)
    %2078 = arith.xori %2052, %true : i1 loc(#loc172)
    %2079 = arith.ori %2078, %2049 : i1
    acir.path %2052 {branch = 52 : i64, kind = "branch", txn = 6 : i64} loc(#loc172)
    acir.path %2052 {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc172)
    %2080 = arith.ori %2078, %2079 : i1 loc(#loc172)
    acir.path %2052 {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc172)
    %2081 = arith.ori %2078, %2080 : i1 loc(#loc172)
    %value_120, %ready_121 = acir.read(%2052) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc168)
    %2082 = arith.andi %2081, %ready_121 : i1 loc(#loc168)
    %2083 = arith.ori %2078, %2082 : i1 loc(#loc168)
    %2084 = arith.andi %2077, %2083 : i1 loc(#loc168)
    %2085 = acir.extract %value_120 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc168)
    %2086 = acir.unary %2085 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc169)
    %2087 = arith.ori %2078, %2081 : i1
    %2088 = arith.andi %2084, %2087 : i1
    %2089 = arith.andi %2052, %2082 : i1
    %2090 = arith.andi %2080, %2089 : i1 loc(#loc170)
    %2091 = arith.andi %2052, %2090 : i1 loc(#loc170)
    %2092 = arith.andi %2091, %2086 : i1 loc(#loc170)
    %2093 = arith.andi %2079, %2092 : i1 loc(#loc170)
    %2094 = arith.ori %2078, %2093 : i1 loc(#loc170)
    %2095 = arith.andi %2088, %2094 : i1 loc(#loc170)
    %2096 = arith.andi %2093, %2040 : i1 loc(#loc170)
    %2097 = arith.andi %2052, %2096 : i1 loc(#loc170)
    %2098 = arith.andi %2094, %2097 : i1 loc(#loc171)
    %2099 = arith.ori %2078, %2098 : i1 loc(#loc171)
    %2100 = arith.andi %2095, %2099 : i1 loc(#loc171)
    %2101 = acir.fire "top.dispatch.dispatch#6" %2100 {successors = [], txn = 6 : i64}
    %2102 = arith.andi %2101, %1200 : i1
    acir.effect %2102() {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "pop", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> () loc(#loc105)
    acir.effect %2102(%1220) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc109)
    %2103 = arith.andi %2101, %1329 : i1
    acir.effect %2103(%0) {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2104 = arith.andi %2101, %1320 : i1
    acir.effect %2104(%0) {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2105 = arith.andi %2101, %1311 : i1
    acir.effect %2105(%0) {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2106 = arith.andi %2101, %1396 : i1
    acir.effect %2106(%0) {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2107 = arith.andi %2101, %1294 : i1
    acir.effect %2107(%1422) {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2102(%1533) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2102(%1540) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc109)
    %2108 = arith.andi %2101, %1644 : i1
    acir.effect %2108(%0) {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2109 = arith.andi %2101, %1635 : i1
    acir.effect %2109(%0) {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2110 = arith.andi %2101, %1626 : i1
    acir.effect %2110(%0) {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2111 = arith.andi %2101, %1711 : i1
    acir.effect %2111(%0) {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2112 = arith.andi %2101, %1610 : i1
    acir.effect %2112(%1737) {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2102(%1847) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2102(%0) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2113 = arith.andi %2101, %1901 : i1
    acir.effect %2113(%0) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate", "top.rob.slot3:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2113(%1925) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc150)
    %2114 = arith.andi %2101, %1899 : i1
    acir.effect %2114(%0) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate", "top.rob.slot2:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2114(%1925) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc150)
    %2115 = arith.andi %2101, %1890 : i1
    acir.effect %2115(%0) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate", "top.rob.slot1:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2115(%1925) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc150)
    %2116 = arith.andi %2101, %1881 : i1
    acir.effect %2116(%0) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate", "top.rob.slot0:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2116(%1925) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc150)
    acir.effect %2102(%2007) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "write", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1, i32) -> () loc(#loc155)
    acir.effect %2102(%0) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2102(%2016) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc159)
    %2117 = arith.andi %2101, %2020 : i1
    acir.effect %2117(%2032) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "write", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1, !acir.array<i32, [8]>) -> () loc(#loc161)
    acir.effect %2102(%0) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %2118 = arith.andi %2101, %2054 : i1
    acir.effect %2118(%0) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2118(%2041) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc171)
    %2119 = arith.andi %2101, %2052 : i1
    acir.effect %2119(%0) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %2119(%2041) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc171)
    acir.path %true {branch = 53 : i64, kind = "branch", txn = 7 : i64} loc(#loc173)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc173)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc174)
    %value_122, %ready_123 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc175)
    %2120 = arith.subi %value_122, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc176)
    %2121 = arith.andi %2120, %c3_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc176)
    %2122 = arith.cmpi eq, %2121, %c0_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc177)
    %2123 = arith.cmpi ne, %2121, %c0_i32 : i32 loc(#loc177)
    %2124 = arith.cmpi eq, %2121, %c0_i32 : i32 loc(#loc177)
    %2125 = arith.ori %2124, %ready_123 : i1
    %2126 = arith.cmpi eq, %2121, %c1_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc178)
    %2127 = arith.andi %2125, %ready_123 : i1
    %2128 = arith.ori %2124, %2127 : i1
    %2129 = arith.andi %ready_123, %2128 : i1
    %2130 = arith.andi %2123, %2126 : i1
    %2131 = arith.cmpi ne, %2121, %c1_i32 : i32 loc(#loc178)
    %2132 = arith.andi %2123, %2131 : i1
    %2133 = arith.xori %2132, %true : i1
    %2134 = arith.ori %2133, %2127 : i1
    %2135 = arith.cmpi eq, %2121, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc179)
    %2136 = arith.andi %2134, %ready_123 : i1
    %2137 = arith.ori %2133, %2136 : i1
    %2138 = arith.andi %2129, %2137 : i1
    %2139 = arith.andi %2132, %2135 : i1
    %2140 = arith.cmpi ne, %2121, %c2_i32 : i32 loc(#loc179)
    %2141 = arith.andi %2132, %2140 : i1
    %2142 = arith.xori %2141, %true : i1 loc(#loc180)
    %2143 = arith.ori %2142, %2136 : i1
    acir.path %2141 {branch = 54 : i64, kind = "branch", txn = 7 : i64} loc(#loc180)
    acir.path %2141 {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc180)
    %2144 = arith.ori %2142, %2143 : i1 loc(#loc180)
    %2145 = arith.andi %2143, %ready_123 : i1 loc(#loc180)
    %2146 = arith.andi %2141, %2145 : i1 loc(#loc180)
    acir.path %2141 {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire", "top.rob.slot3:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc180)
    %2147 = arith.ori %2142, %2144 : i1 loc(#loc180)
    %2148 = arith.andi %2144, %2146 : i1 loc(#loc180)
    %2149 = arith.andi %2141, %2148 : i1 loc(#loc180)
    %value_124, %ready_125 = acir.read(%2141) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire", "top.rob.slot3:completed"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc181)
    %2150 = arith.andi %2147, %ready_125 : i1 loc(#loc181)
    %2151 = arith.ori %2142, %2150 : i1 loc(#loc181)
    %2152 = arith.andi %2138, %2151 : i1 loc(#loc181)
    %2153 = acir.extract %value_124 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc182)
    %2154 = arith.andi %2152, %2151 : i1
    %2155 = arith.andi %2141, %2153 : i1
    %2156 = arith.xori %2153, %true : i1
    %2157 = arith.andi %2141, %2156 : i1
    %2158 = arith.xori %2157, %true : i1
    %2159 = arith.ori %2158, %2150 : i1
    %2160 = arith.xori %2155, %true : i1
    %2161 = arith.ori %2160, %2150 : i1
    %2162 = acir.extract %value_124 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc183)
    %2163 = arith.andi %2157, %2159 : i1
    %2164 = arith.andi %2161, %ready_125 : i1
    %2165 = arith.andi %2155, %2164 : i1
    %2166 = arith.ori %2163, %2165 : i1
    %2167 = arith.select %2157, %false, %2162 : i1
    %2168 = arith.ori %2142, %2147 : i1
    %2169 = arith.andi %2168, %2166 : i1
    %2170 = arith.ori %2142, %2169 : i1
    %2171 = arith.andi %2154, %2170 : i1
    %2172 = arith.andi %2141, %2167 : i1
    %2173 = arith.xori %2167, %true : i1
    %2174 = arith.andi %2141, %2173 : i1
    %2175 = arith.xori %2174, %true : i1
    %2176 = arith.ori %2175, %2169 : i1
    %2177 = arith.xori %2172, %true : i1
    %2178 = arith.ori %2177, %2169 : i1
    %2179 = acir.extract %value_124 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc184)
    %2180 = arith.andi %ready_125, %2149 : i1 loc(#loc184)
    %2181 = arith.cmpi eq, %2179, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc184)
    %2182 = arith.andi %2174, %2176 : i1
    %2183 = arith.andi %2178, %2180 : i1
    %2184 = arith.andi %2172, %2183 : i1
    %2185 = arith.ori %2182, %2184 : i1
    %2186 = arith.select %2174, %false, %2181 : i1
    %2187 = arith.ori %2142, %2168 : i1
    %2188 = arith.ori %2142, %2187 : i1
    %2189 = arith.andi %2171, %2188 : i1
    %2190 = arith.andi %2187, %2185 : i1
    %2191 = arith.andi %2141, %2190 : i1
    %2192 = arith.andi %2144, %2191 : i1 loc(#loc185)
    %2193 = arith.andi %2141, %2192 : i1 loc(#loc185)
    %2194 = arith.andi %2193, %2186 : i1 loc(#loc185)
    %2195 = arith.andi %2143, %2194 : i1 loc(#loc185)
    %2196 = arith.ori %2142, %2195 : i1 loc(#loc185)
    %2197 = arith.andi %2189, %2196 : i1 loc(#loc185)
    %value_126, %ready_127 = acir.read(%2141) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc186)
    %2198 = arith.andi %2196, %ready_127 : i1 loc(#loc186)
    %2199 = arith.ori %2142, %2198 : i1 loc(#loc186)
    %2200 = arith.andi %2197, %2199 : i1 loc(#loc186)
    %2201 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc187)
    %2202 = arith.ori %2142, %2196 : i1 loc(#loc188)
    %2203 = arith.andi %2200, %2202 : i1 loc(#loc188)
    %2204 = arith.andi %2203, %2202 : i1
    %2205 = arith.andi %2141, %2198 : i1
    %2206 = arith.andi %2205, %2194 : i1
    %2207 = arith.xori %2139, %true : i1 loc(#loc189)
    %2208 = arith.ori %2207, %2136 : i1
    acir.path %2139 {branch = 55 : i64, kind = "branch", txn = 7 : i64} loc(#loc189)
    acir.path %2139 {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc189)
    %2209 = arith.ori %2207, %2208 : i1 loc(#loc189)
    %2210 = arith.andi %2208, %ready_123 : i1 loc(#loc189)
    %2211 = arith.andi %2139, %2210 : i1 loc(#loc189)
    acir.path %2139 {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire", "top.rob.slot2:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc189)
    %2212 = arith.ori %2207, %2209 : i1 loc(#loc189)
    %2213 = arith.andi %2209, %2211 : i1 loc(#loc189)
    %2214 = arith.andi %2139, %2213 : i1 loc(#loc189)
    %value_128, %ready_129 = acir.read(%2139) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire", "top.rob.slot2:completed"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc181)
    %2215 = arith.andi %2212, %ready_129 : i1 loc(#loc181)
    %2216 = arith.ori %2207, %2215 : i1 loc(#loc181)
    %2217 = arith.andi %2204, %2216 : i1 loc(#loc181)
    %2218 = acir.extract %value_128 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc182)
    %2219 = arith.andi %2217, %2216 : i1
    %2220 = arith.andi %2139, %2218 : i1
    %2221 = arith.xori %2218, %true : i1
    %2222 = arith.andi %2139, %2221 : i1
    %2223 = arith.xori %2222, %true : i1
    %2224 = arith.ori %2223, %2215 : i1
    %2225 = arith.xori %2220, %true : i1
    %2226 = arith.ori %2225, %2215 : i1
    %2227 = acir.extract %value_128 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc183)
    %2228 = arith.andi %2222, %2224 : i1
    %2229 = arith.andi %2226, %ready_129 : i1
    %2230 = arith.andi %2220, %2229 : i1
    %2231 = arith.ori %2228, %2230 : i1
    %2232 = arith.select %2222, %false, %2227 : i1
    %2233 = arith.ori %2207, %2212 : i1
    %2234 = arith.andi %2233, %2231 : i1
    %2235 = arith.ori %2207, %2234 : i1
    %2236 = arith.andi %2219, %2235 : i1
    %2237 = arith.andi %2139, %2232 : i1
    %2238 = arith.xori %2232, %true : i1
    %2239 = arith.andi %2139, %2238 : i1
    %2240 = arith.xori %2239, %true : i1
    %2241 = arith.ori %2240, %2234 : i1
    %2242 = arith.xori %2237, %true : i1
    %2243 = arith.ori %2242, %2234 : i1
    %2244 = acir.extract %value_128 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc184)
    %2245 = arith.andi %ready_129, %2214 : i1 loc(#loc184)
    %2246 = arith.cmpi eq, %2244, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc184)
    %2247 = arith.andi %2239, %2241 : i1
    %2248 = arith.andi %2243, %2245 : i1
    %2249 = arith.andi %2237, %2248 : i1
    %2250 = arith.ori %2247, %2249 : i1
    %2251 = arith.select %2239, %false, %2246 : i1
    %2252 = arith.ori %2207, %2233 : i1
    %2253 = arith.ori %2207, %2252 : i1
    %2254 = arith.andi %2236, %2253 : i1
    %2255 = arith.andi %2252, %2250 : i1
    %2256 = arith.andi %2139, %2255 : i1
    %2257 = arith.andi %2209, %2256 : i1 loc(#loc185)
    %2258 = arith.andi %2139, %2257 : i1 loc(#loc185)
    %2259 = arith.andi %2258, %2251 : i1 loc(#loc185)
    %2260 = arith.andi %2208, %2259 : i1 loc(#loc185)
    %2261 = arith.ori %2207, %2260 : i1 loc(#loc185)
    %2262 = arith.andi %2254, %2261 : i1 loc(#loc185)
    %value_130, %ready_131 = acir.read(%2139) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc186)
    %2263 = arith.andi %2261, %ready_131 : i1 loc(#loc186)
    %2264 = arith.ori %2207, %2263 : i1 loc(#loc186)
    %2265 = arith.andi %2262, %2264 : i1 loc(#loc186)
    %2266 = arith.ori %2207, %2261 : i1 loc(#loc188)
    %2267 = arith.andi %2265, %2266 : i1 loc(#loc188)
    %2268 = arith.andi %2267, %2266 : i1
    %2269 = arith.andi %2139, %2263 : i1
    %2270 = arith.andi %2269, %2259 : i1
    %2271 = arith.andi %2143, %2206 : i1
    %2272 = arith.andi %2141, %2271 : i1
    %2273 = arith.andi %2208, %2270 : i1
    %2274 = arith.andi %2139, %2273 : i1
    %2275 = arith.ori %2272, %2274 : i1
    %2276 = arith.select %2141, %value_126, %value_130 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2277 = arith.ori %2133, %2134 : i1
    %2278 = arith.xori %2130, %true : i1 loc(#loc190)
    %2279 = arith.ori %2278, %2127 : i1
    acir.path %2130 {branch = 56 : i64, kind = "branch", txn = 7 : i64} loc(#loc190)
    acir.path %2130 {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc190)
    %2280 = arith.ori %2278, %2279 : i1 loc(#loc190)
    %2281 = arith.andi %2279, %ready_123 : i1 loc(#loc190)
    %2282 = arith.andi %2130, %2281 : i1 loc(#loc190)
    acir.path %2130 {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire", "top.rob.slot1:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc190)
    %2283 = arith.ori %2278, %2280 : i1 loc(#loc190)
    %2284 = arith.andi %2280, %2282 : i1 loc(#loc190)
    %2285 = arith.andi %2130, %2284 : i1 loc(#loc190)
    %value_132, %ready_133 = acir.read(%2130) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire", "top.rob.slot1:completed"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc181)
    %2286 = arith.andi %2283, %ready_133 : i1 loc(#loc181)
    %2287 = arith.ori %2278, %2286 : i1 loc(#loc181)
    %2288 = arith.andi %2268, %2287 : i1 loc(#loc181)
    %2289 = acir.extract %value_132 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc182)
    %2290 = arith.andi %2288, %2287 : i1
    %2291 = arith.andi %2130, %2289 : i1
    %2292 = arith.xori %2289, %true : i1
    %2293 = arith.andi %2130, %2292 : i1
    %2294 = arith.xori %2293, %true : i1
    %2295 = arith.ori %2294, %2286 : i1
    %2296 = arith.xori %2291, %true : i1
    %2297 = arith.ori %2296, %2286 : i1
    %2298 = acir.extract %value_132 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc183)
    %2299 = arith.andi %2293, %2295 : i1
    %2300 = arith.andi %2297, %ready_133 : i1
    %2301 = arith.andi %2291, %2300 : i1
    %2302 = arith.ori %2299, %2301 : i1
    %2303 = arith.select %2293, %false, %2298 : i1
    %2304 = arith.ori %2278, %2283 : i1
    %2305 = arith.andi %2304, %2302 : i1
    %2306 = arith.ori %2278, %2305 : i1
    %2307 = arith.andi %2290, %2306 : i1
    %2308 = arith.andi %2130, %2303 : i1
    %2309 = arith.xori %2303, %true : i1
    %2310 = arith.andi %2130, %2309 : i1
    %2311 = arith.xori %2310, %true : i1
    %2312 = arith.ori %2311, %2305 : i1
    %2313 = arith.xori %2308, %true : i1
    %2314 = arith.ori %2313, %2305 : i1
    %2315 = acir.extract %value_132 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc184)
    %2316 = arith.andi %ready_133, %2285 : i1 loc(#loc184)
    %2317 = arith.cmpi eq, %2315, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc184)
    %2318 = arith.andi %2310, %2312 : i1
    %2319 = arith.andi %2314, %2316 : i1
    %2320 = arith.andi %2308, %2319 : i1
    %2321 = arith.ori %2318, %2320 : i1
    %2322 = arith.select %2310, %false, %2317 : i1
    %2323 = arith.ori %2278, %2304 : i1
    %2324 = arith.ori %2278, %2323 : i1
    %2325 = arith.andi %2307, %2324 : i1
    %2326 = arith.andi %2323, %2321 : i1
    %2327 = arith.andi %2130, %2326 : i1
    %2328 = arith.andi %2280, %2327 : i1 loc(#loc185)
    %2329 = arith.andi %2130, %2328 : i1 loc(#loc185)
    %2330 = arith.andi %2329, %2322 : i1 loc(#loc185)
    %2331 = arith.andi %2279, %2330 : i1 loc(#loc185)
    %2332 = arith.ori %2278, %2331 : i1 loc(#loc185)
    %2333 = arith.andi %2325, %2332 : i1 loc(#loc185)
    %value_134, %ready_135 = acir.read(%2130) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc186)
    %2334 = arith.andi %2332, %ready_135 : i1 loc(#loc186)
    %2335 = arith.ori %2278, %2334 : i1 loc(#loc186)
    %2336 = arith.andi %2333, %2335 : i1 loc(#loc186)
    %2337 = arith.ori %2278, %2332 : i1 loc(#loc188)
    %2338 = arith.andi %2336, %2337 : i1 loc(#loc188)
    %2339 = arith.andi %2338, %2337 : i1
    %2340 = arith.andi %2130, %2334 : i1
    %2341 = arith.andi %2340, %2330 : i1
    %2342 = arith.andi %2277, %2275 : i1
    %2343 = arith.andi %2132, %2342 : i1
    %2344 = arith.andi %2279, %2341 : i1
    %2345 = arith.andi %2130, %2344 : i1
    %2346 = arith.ori %2343, %2345 : i1
    %2347 = arith.select %2132, %2276, %value_134 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2348 = arith.ori %2124, %2125 : i1
    %2349 = arith.ori %2123, %ready_123 : i1
    acir.path %2122 {branch = 57 : i64, kind = "branch", txn = 7 : i64} loc(#loc191)
    acir.path %2122 {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc191)
    %2350 = arith.ori %2123, %2349 : i1 loc(#loc191)
    %2351 = arith.andi %2349, %ready_123 : i1 loc(#loc191)
    %2352 = arith.andi %2122, %2351 : i1 loc(#loc191)
    acir.path %2122 {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire", "top.rob.slot0:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc191)
    %2353 = arith.ori %2123, %2350 : i1 loc(#loc191)
    %2354 = arith.andi %2350, %2352 : i1 loc(#loc191)
    %2355 = arith.andi %2122, %2354 : i1 loc(#loc191)
    %value_136, %ready_137 = acir.read(%2122) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire", "top.rob.slot0:completed"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc181)
    %2356 = arith.andi %2353, %ready_137 : i1 loc(#loc181)
    %2357 = arith.ori %2123, %2356 : i1 loc(#loc181)
    %2358 = arith.andi %2339, %2357 : i1 loc(#loc181)
    %2359 = acir.extract %value_136 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc182)
    %2360 = arith.andi %2358, %2357 : i1
    %2361 = arith.andi %2122, %2359 : i1
    %2362 = arith.xori %2359, %true : i1
    %2363 = arith.andi %2122, %2362 : i1
    %2364 = arith.xori %2363, %true : i1
    %2365 = arith.ori %2364, %2356 : i1
    %2366 = arith.xori %2361, %true : i1
    %2367 = arith.ori %2366, %2356 : i1
    %2368 = acir.extract %value_136 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc183)
    %2369 = arith.andi %2363, %2365 : i1
    %2370 = arith.andi %2367, %ready_137 : i1
    %2371 = arith.andi %2361, %2370 : i1
    %2372 = arith.ori %2369, %2371 : i1
    %2373 = arith.select %2363, %false, %2368 : i1
    %2374 = arith.ori %2123, %2353 : i1
    %2375 = arith.andi %2374, %2372 : i1
    %2376 = arith.ori %2123, %2375 : i1
    %2377 = arith.andi %2360, %2376 : i1
    %2378 = arith.andi %2122, %2373 : i1
    %2379 = arith.xori %2373, %true : i1
    %2380 = arith.andi %2122, %2379 : i1
    %2381 = arith.xori %2380, %true : i1
    %2382 = arith.ori %2381, %2375 : i1
    %2383 = arith.xori %2378, %true : i1
    %2384 = arith.ori %2383, %2375 : i1
    %2385 = acir.extract %value_136 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc184)
    %2386 = arith.andi %ready_137, %2355 : i1 loc(#loc184)
    %2387 = arith.cmpi eq, %2385, %value_122 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc184)
    %2388 = arith.andi %2380, %2382 : i1
    %2389 = arith.andi %2384, %2386 : i1
    %2390 = arith.andi %2378, %2389 : i1
    %2391 = arith.ori %2388, %2390 : i1
    %2392 = arith.select %2380, %false, %2387 : i1
    %2393 = arith.ori %2123, %2374 : i1
    %2394 = arith.ori %2123, %2393 : i1
    %2395 = arith.andi %2377, %2394 : i1
    %2396 = arith.andi %2393, %2391 : i1
    %2397 = arith.andi %2122, %2396 : i1
    %2398 = arith.andi %2350, %2397 : i1 loc(#loc185)
    %2399 = arith.andi %2122, %2398 : i1 loc(#loc185)
    %2400 = arith.andi %2399, %2392 : i1 loc(#loc185)
    %2401 = arith.andi %2349, %2400 : i1 loc(#loc185)
    %2402 = arith.ori %2123, %2401 : i1 loc(#loc185)
    %2403 = arith.andi %2395, %2402 : i1 loc(#loc185)
    %value_138, %ready_139 = acir.read(%2122) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc186)
    %2404 = arith.andi %2402, %ready_139 : i1 loc(#loc186)
    %2405 = arith.ori %2123, %2404 : i1 loc(#loc186)
    %2406 = arith.andi %2403, %2405 : i1 loc(#loc186)
    %2407 = arith.ori %2123, %2402 : i1 loc(#loc188)
    %2408 = arith.andi %2406, %2407 : i1 loc(#loc188)
    %2409 = arith.andi %2408, %2407 : i1
    %2410 = arith.andi %2122, %2404 : i1
    %2411 = arith.andi %2410, %2400 : i1
    %2412 = arith.andi %2348, %2346 : i1
    %2413 = arith.andi %2123, %2412 : i1
    %2414 = arith.andi %2349, %2411 : i1
    %2415 = arith.andi %2122, %2414 : i1
    %2416 = arith.ori %2413, %2415 : i1
    %2417 = arith.select %2123, %2347, %value_138 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2418 = arith.addi %value_122, %c1_i32 {acir.id = "v35", acir.source_type = "u32"} : i32 loc(#loc192)
    %2419 = arith.andi %2409, %ready_123 : i1 loc(#loc193)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc194)
    %2420 = acir.extract %2417 {acir.id = "v5", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc195)
    %2421 = arith.cmpi ne, %2420, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc195)
    %2422 = arith.andi %2419, %2416 : i1
    %2423 = arith.cmpi eq, %2420, %c0_i32 : i32 loc(#loc195)
    %2424 = arith.ori %2423, %2416 : i1
    %2425 = acir.extract %2417 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc196)
    acir.path %2421 {branch = 58 : i64, kind = "branch", txn = 7 : i64} loc(#loc197)
    %value_140, %ready_141 = acir.read(%2421) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc197)
    %2426 = arith.andi %2424, %ready_141 : i1 loc(#loc197)
    %2427 = arith.ori %2423, %2426 : i1 loc(#loc197)
    %2428 = arith.andi %2422, %2427 : i1 loc(#loc197)
    %2429 = acir.extract %2417 {acir.id = "v13", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc198)
    %2430 = arith.andi %ready_141, %2416 : i1 loc(#loc197)
    %2431 = arith.extui %2429 : i32 to i64 loc(#loc197)
    %2432 = arith.cmpi ult, %2431, %c8_i64 : i64 loc(#loc197)
    %2433 = arith.andi %2430, %2432 : i1 loc(#loc197)
    %2434 = arith.select %2432, %2431, %c0_i64 : i64 loc(#loc197)
    %2435 = acir.update %value_140, %2425[%2434] {acir.id = "v14", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i64) -> !acir.array<i32, [8]> loc(#loc197)
    %2436 = arith.andi %2424, %2433 : i1 loc(#loc197)
    %2437 = arith.ori %2423, %2436 : i1 loc(#loc197)
    %2438 = arith.andi %2428, %2437 : i1 loc(#loc197)
    %2439 = acir.extract %2417 {acir.id = "v17", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc199)
    %value_142, %ready_143 = acir.read(%2421) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1) -> (!acir.array<i32, [8]>, i1) loc(#loc200)
    %2440 = arith.andi %2424, %ready_143 : i1 loc(#loc200)
    %2441 = arith.ori %2423, %2440 : i1 loc(#loc200)
    %2442 = arith.andi %2438, %2441 : i1 loc(#loc200)
    %2443 = acir.extract %2417 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc201)
    %2444 = arith.andi %ready_143, %2416 : i1 loc(#loc200)
    %2445 = arith.extui %2443 : i32 to i64 loc(#loc200)
    %2446 = arith.cmpi ult, %2445, %c8_i64 : i64 loc(#loc200)
    %2447 = arith.andi %2444, %2446 : i1 loc(#loc200)
    %2448 = arith.select %2446, %2445, %c0_i64 : i64 loc(#loc200)
    %2449 = acir.update %value_142, %2439[%2448] {acir.id = "v22", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i64) -> !acir.array<i32, [8]> loc(#loc200)
    %2450 = arith.andi %2424, %2447 : i1 loc(#loc200)
    %2451 = arith.ori %2423, %2450 : i1 loc(#loc200)
    %2452 = arith.andi %2442, %2451 : i1 loc(#loc200)
    %value_144, %ready_145 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit"], method = "read", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc202)
    %2453 = arith.andi %2452, %ready_145 : i1 loc(#loc202)
    %2454 = arith.addi %value_144, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc202)
    %2455 = acir.extract %2417 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc203)
    %2456 = arith.andi %2453, %2416 : i1 loc(#loc204)
    %2457 = acir.fire "top.commit.commit#7" %2456 {successors = [], txn = 7 : i64}
    %2458 = arith.andi %2457, %2141 : i1
    acir.effect %2458(%0) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire", "top.rob.slot3:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2458(%2201) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc188)
    acir.effect %2458(%0) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2459 = arith.andi %2457, %2139 : i1
    acir.effect %2459(%0) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire", "top.rob.slot2:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2459(%2201) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc188)
    acir.effect %2459(%0) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2460 = arith.andi %2457, %2130 : i1
    acir.effect %2460(%0) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire", "top.rob.slot1:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2460(%2201) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc188)
    acir.effect %2460(%0) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2461 = arith.andi %2457, %2122 : i1
    acir.effect %2461(%0) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire", "top.rob.slot0:completed"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2461(%2201) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc188)
    acir.effect %2461(%0) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2457(%2418) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "write", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1, i32) -> () loc(#loc193)
    acir.effect %2457(%0) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2462 = arith.andi %2457, %2421 : i1
    acir.effect %2462(%2435) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1, !acir.array<i32, [8]>) -> () loc(#loc197)
    acir.effect %2462(%2449) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1, !acir.array<i32, [8]>) -> () loc(#loc200)
    acir.effect %2457(%2454) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1, i32) -> () loc(#loc205)
    acir.effect %2457(%2455) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.last_tag", txn = 7 : i64} : (i1, i32) -> () loc(#loc204)
    acir.path %true {branch = 59 : i64, kind = "branch", txn = 8 : i64} loc(#loc206)
    acir.path %true {branch = 59 : i64, frames = ["top.audit:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 8 : i64} loc(#loc206)
    %value_146, %ready_147 = acir.read(%true) {branch = 59 : i64, frames = ["top.audit:observe"], method = "get", port = 0 : i64, resource = "top.event", txn = 8 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc207)
    %2463 = acir.extract %value_146 {acir.id = "v6", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc208)
    %2464 = acir.fire "top.audit.observe#8" %ready_147 {successors = [], txn = 8 : i64}
    acir.effect %2464(%2463) {branch = 59 : i64, frames = ["top.audit:observe"], method = "write", port = 0 : i64, resource = "top.audit.last_tag", txn = 8 : i64} : (i1, i32) -> () loc(#loc209)
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
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:17)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:12)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:14)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:14)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:13)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:17)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:12)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:33)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:29)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:50)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:13)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:13)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":67:13)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:13)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":136:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:13)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:17)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:12)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:20)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:16)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:40)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:29)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:17)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:20)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:16)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:40)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:29)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:17)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":42:13)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:16)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:9)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":168:12)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:15)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:20)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:19)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:12)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":172:39)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":172:20)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":173:17)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:17)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:12)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:14)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":126:14)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:20)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:20)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:20)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:16)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:12)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:28)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:44)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:39)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:20)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:16)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:16)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:23)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:20)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:15)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:17)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:12)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:14)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:14)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":115:13)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:20)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:16)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":58:20)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:9)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:13)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":111:13)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":109:13)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:16)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:9)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:16)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:9)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:12)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:13)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:20)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:16)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":15:20)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":17:9)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":70:9)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":64:17)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:15)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:17)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:12)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:14)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":150:14)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:21)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:17)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:16)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:32)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:48)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":79:20)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":81:17)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:17)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:9)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":151:21)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:21)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:21)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:16)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:9)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:9)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":186:12)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":187:35)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":187:13)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":187:23)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":188:35)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":188:13)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":188:23)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:19)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:20)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:9)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":80:17)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:20)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:9)

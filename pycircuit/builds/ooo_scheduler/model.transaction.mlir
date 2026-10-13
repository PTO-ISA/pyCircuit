module attributes {acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, exports = {}, format_version = 8 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", type = "bool"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default, name = "lhs", type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", type = "bool"}, {default = false, name = "done", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}]}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.adder.out", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.adder.out", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cdb.event", source_branch = 29 : i64, target_branches = [59], to = 8 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.multiplier.out", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.multiplier.out", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.scheduler.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}], schedule = [3, 1, 5, 2, 0, 4, 6, 7, 8], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.adder.out : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.audit.last_tag : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cdb.event : !acir.struct<"src_payloads_25ebcaa7.Completion"> {initial = [0 : i32, 0 : i32, 0 : i32], kind = "signal"}
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
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.left.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc1)
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.right.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc2)
    %value_2, %ready_3 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.adder.out", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc3)
    %value_4, %ready_5 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.multiplier.out", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc4)
    %value_6, %ready_7 = acir.read(%true) {branch = 0 : i64, frames = [], method = "get", port = 0 : i64, resource = "top.cdb.event", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc5)
    %value_8, %ready_9 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.audit.last_tag", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc6)
    %value_10, %ready_11 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc7)
    %value_12, %ready_13 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc8)
    %value_14, %ready_15 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc9)
    %value_16, %ready_17 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc10)
    %true_18 = arith.constant {acir.source_type = "bool"} true
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_19 = arith.constant {acir.source_type = "bool"} false
    %value_20, %ready_21 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.left.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc11)
    %value_22, %ready_23 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.right.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc12)
    %0 = acir.extract %value_20 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc13)
    %1 = arith.xori %0, %true : i1
    %2 = arith.xori %1, %true : i1
    %3 = arith.ori %2, %ready_21 : i1
    %4 = arith.xori %0, %true : i1
    %5 = arith.ori %4, %ready_21 : i1
    %6 = acir.extract %value_20 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc14)
    %7 = acir.extract %6 {acir.id = "v8", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc14)
    %8 = arith.xori %1, %true : i1
    %9 = arith.ori %8, %3 : i1
    %10 = arith.ori %1, %0 : i1
    %11 = arith.xori %0, %true : i1
    %12 = arith.ori %11, %5 : i1
    %13 = arith.andi %9, %12 : i1
    %14 = arith.andi %1, %3 : i1
    %15 = arith.andi %5, %ready_21 : i1
    %16 = arith.andi %0, %15 : i1
    %17 = arith.ori %14, %16 : i1
    %18 = arith.select %1, %false_19, %7 : i1
    %19 = arith.xori %18, %true : i1
    %20 = arith.xori %19, %true : i1
    %21 = arith.ori %20, %17 : i1
    %22 = arith.xori %18, %true : i1
    %23 = arith.ori %22, %17 : i1
    %24 = acir.extract %value_20 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc15)
    %25 = acir.extract %24 {acir.id = "v15", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc15)
    %26 = arith.xori %19, %true : i1
    %27 = arith.ori %26, %21 : i1
    %28 = arith.ori %19, %18 : i1
    %29 = arith.xori %18, %true : i1
    %30 = arith.ori %29, %23 : i1
    %31 = arith.andi %27, %30 : i1
    %32 = arith.andi %19, %21 : i1
    %33 = arith.andi %23, %ready_21 : i1
    %34 = arith.andi %18, %33 : i1
    %35 = arith.ori %32, %34 : i1
    %36 = arith.select %19, %false_19, %25 : i1
    %37 = acir.extract %value_22 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc16)
    %38 = arith.xori %37, %true : i1
    %39 = arith.xori %38, %true : i1
    %40 = arith.ori %39, %ready_23 : i1
    %41 = arith.xori %37, %true : i1
    %42 = arith.ori %41, %ready_23 : i1
    %43 = acir.extract %value_22 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc17)
    %44 = acir.extract %43 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc17)
    %45 = arith.xori %38, %true : i1
    %46 = arith.ori %45, %40 : i1
    %47 = arith.ori %38, %37 : i1
    %48 = arith.xori %37, %true : i1
    %49 = arith.ori %48, %42 : i1
    %50 = arith.andi %46, %49 : i1
    %51 = arith.andi %38, %40 : i1
    %52 = arith.andi %42, %ready_23 : i1
    %53 = arith.andi %37, %52 : i1
    %54 = arith.ori %51, %53 : i1
    %55 = arith.select %38, %false_19, %44 : i1
    %56 = arith.xori %55, %true : i1
    %57 = arith.xori %56, %true : i1
    %58 = arith.ori %57, %54 : i1
    %59 = arith.xori %55, %true : i1
    %60 = arith.ori %59, %54 : i1
    %61 = acir.extract %value_22 {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc18)
    %62 = acir.extract %61 {acir.id = "v30", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc18)
    %63 = arith.xori %56, %true : i1
    %64 = arith.ori %63, %58 : i1
    %65 = arith.ori %56, %55 : i1
    %66 = arith.xori %55, %true : i1
    %67 = arith.ori %66, %60 : i1
    %68 = arith.andi %64, %67 : i1
    %69 = arith.andi %56, %58 : i1
    %70 = arith.andi %60, %ready_23 : i1
    %71 = arith.andi %55, %70 : i1
    %72 = arith.ori %69, %71 : i1
    %73 = arith.select %56, %false_19, %62 : i1
    %74 = arith.xori %36, %true : i1
    %75 = arith.xori %74, %true : i1
    %76 = arith.ori %75, %35 : i1
    %77 = arith.xori %36, %true : i1
    %78 = arith.ori %77, %35 : i1
    %79 = acir.extract %value_20 {acir.id = "v36", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc19)
    %80 = arith.cmpi eq, %79, %c0_i32 {acir.id = "v38", acir.source_type = "bool"} : i32 loc(#loc19)
    %81 = arith.xori %74, %true : i1
    %82 = arith.ori %81, %76 : i1
    %83 = arith.ori %74, %36 : i1
    %84 = arith.xori %36, %true : i1
    %85 = arith.ori %84, %78 : i1
    %86 = arith.andi %82, %85 : i1
    %87 = arith.andi %74, %76 : i1
    %88 = arith.andi %78, %ready_21 : i1
    %89 = arith.andi %36, %88 : i1
    %90 = arith.ori %87, %89 : i1
    %91 = arith.select %74, %false_19, %80 : i1
    %92 = arith.xori %73, %true : i1
    %93 = arith.xori %92, %true : i1
    %94 = arith.ori %93, %72 : i1
    %95 = arith.xori %73, %true : i1
    %96 = arith.ori %95, %72 : i1
    %97 = acir.extract %value_22 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc20)
    %98 = arith.cmpi eq, %97, %c0_i32 {acir.id = "v46", acir.source_type = "bool"} : i32 loc(#loc20)
    %99 = arith.xori %92, %true : i1
    %100 = arith.ori %99, %94 : i1
    %101 = arith.ori %92, %73 : i1
    %102 = arith.xori %73, %true : i1
    %103 = arith.ori %102, %96 : i1
    %104 = arith.andi %100, %103 : i1
    %105 = arith.andi %92, %94 : i1
    %106 = arith.andi %96, %ready_23 : i1
    %107 = arith.andi %73, %106 : i1
    %108 = arith.ori %105, %107 : i1
    %109 = arith.select %92, %false_19, %98 : i1
    %110 = arith.xori %36, %true : i1
    %111 = arith.xori %110, %true : i1
    %112 = arith.ori %111, %35 : i1
    %113 = arith.xori %36, %true : i1
    %114 = arith.ori %113, %35 : i1
    %115 = acir.extract %value_20 {acir.id = "v52", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc21)
    %116 = arith.cmpi eq, %115, %c1_i32 {acir.id = "v54", acir.source_type = "bool"} : i32 loc(#loc21)
    %117 = arith.xori %110, %true : i1
    %118 = arith.ori %117, %112 : i1
    %119 = arith.ori %110, %36 : i1
    %120 = arith.xori %36, %true : i1
    %121 = arith.ori %120, %114 : i1
    %122 = arith.andi %118, %121 : i1
    %123 = arith.andi %110, %112 : i1
    %124 = arith.andi %114, %ready_21 : i1
    %125 = arith.andi %36, %124 : i1
    %126 = arith.ori %123, %125 : i1
    %127 = arith.select %110, %false_19, %116 : i1
    %128 = arith.xori %73, %true : i1
    %129 = arith.xori %128, %true : i1
    %130 = arith.ori %129, %72 : i1
    %131 = arith.xori %73, %true : i1
    %132 = arith.ori %131, %72 : i1
    %133 = acir.extract %value_22 {acir.id = "v60", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc22)
    %134 = arith.cmpi eq, %133, %c1_i32 {acir.id = "v62", acir.source_type = "bool"} : i32 loc(#loc22)
    %135 = arith.xori %128, %true : i1
    %136 = arith.ori %135, %130 : i1
    %137 = arith.ori %128, %73 : i1
    %138 = arith.xori %73, %true : i1
    %139 = arith.ori %138, %132 : i1
    %140 = arith.andi %136, %139 : i1
    %141 = arith.andi %128, %130 : i1
    %142 = arith.andi %132, %ready_23 : i1
    %143 = arith.andi %73, %142 : i1
    %144 = arith.ori %141, %143 : i1
    %145 = arith.select %128, %false_19, %134 : i1
    %146 = arith.xori %91, %true : i1
    %147 = arith.xori %146, %true : i1
    %148 = arith.ori %147, %90 : i1
    %149 = arith.xori %91, %true : i1
    %150 = arith.ori %149, %90 : i1
    %151 = acir.unary %109 "not" {acir.id = "v68", acir.source_type = "bool"} : i1 -> i1 loc(#loc23)
    %152 = arith.andi %91, %151 : i1
    %153 = arith.andi %150, %108 : i1
    %154 = arith.xori %151, %true : i1
    %155 = arith.andi %91, %154 : i1
    %156 = arith.andi %150, %108 : i1
    %157 = arith.xori %155, %true : i1
    %158 = arith.ori %157, %156 : i1
    %159 = acir.extract %value_20 {acir.id = "v71", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc24)
    %160 = acir.extract %value_22 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc25)
    %161 = arith.andi %ready_21, %ready_23 : i1 loc(#loc24)
    %162 = arith.cmpi ult, %159, %160 {acir.id = "v73", acir.source_type = "bool"} : i32 loc(#loc24)
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
    %175 = arith.select %155, %162, %true_18 : i1
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
    %188 = arith.select %146, %false_19, %175 : i1
    %189 = arith.xori %127, %true : i1
    %190 = arith.xori %189, %true : i1
    %191 = arith.ori %190, %126 : i1
    %192 = arith.xori %127, %true : i1
    %193 = arith.ori %192, %126 : i1
    %194 = acir.unary %145 "not" {acir.id = "v82", acir.source_type = "bool"} : i1 -> i1 loc(#loc26)
    %195 = arith.andi %127, %194 : i1
    %196 = arith.andi %193, %144 : i1
    %197 = arith.xori %194, %true : i1
    %198 = arith.andi %127, %197 : i1
    %199 = arith.andi %193, %144 : i1
    %200 = arith.xori %198, %true : i1
    %201 = arith.ori %200, %199 : i1
    %202 = acir.extract %value_20 {acir.id = "v85", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc27)
    %203 = acir.extract %value_22 {acir.id = "v86", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc28)
    %204 = arith.andi %ready_21, %ready_23 : i1 loc(#loc27)
    %205 = arith.cmpi ult, %202, %203 {acir.id = "v87", acir.source_type = "bool"} : i32 loc(#loc27)
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
    %218 = arith.select %198, %205, %true_18 : i1
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
    %231 = arith.select %189, %false_19, %218 : i1
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc29)
    acir.path %true {branch = 1 : i64, frames = ["top.scheduler:issue_add"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc29)
    %c0_i32_24 = arith.constant {acir.source_type = "u32"} 0 : i32
    %232 = arith.andi %187, %187 : i1
    %233 = arith.xori %188, %true : i1
    %234 = arith.xori %233, %true : i1
    %235 = arith.ori %234, %187 : i1
    acir.path %233 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc30)
    acir.path %233 {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc30)
    %236 = arith.xori %233, %true : i1 loc(#loc30)
    %237 = arith.ori %236, %235 : i1 loc(#loc30)
    %238 = arith.andi %233, %235 : i1 loc(#loc30)
    %239 = arith.xori %233, %true : i1 loc(#loc30)
    %240 = arith.ori %239, %237 : i1 loc(#loc30)
    %241 = arith.andi %237, %238 : i1 loc(#loc30)
    %242 = arith.andi %233, %241 : i1 loc(#loc30)
    %false_25 = arith.constant {acir.source_type = "bool"} false
    %value_26, %ready_27 = acir.read(%233) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc31)
    %243 = arith.xori %233, %true : i1 loc(#loc31)
    %244 = arith.andi %240, %ready_27 : i1 loc(#loc31)
    %245 = arith.ori %243, %244 : i1 loc(#loc31)
    %246 = arith.andi %232, %245 : i1 loc(#loc31)
    %247 = acir.extract %value_26 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc32)
    %248 = arith.xori %233, %true : i1
    %249 = arith.andi %240, %ready_27 : i1
    %250 = arith.ori %248, %249 : i1
    %251 = arith.andi %246, %250 : i1
    %252 = arith.andi %233, %247 : i1
    %253 = arith.andi %240, %ready_27 : i1
    %254 = arith.xori %247, %true : i1
    %255 = arith.andi %233, %254 : i1
    %256 = arith.andi %240, %ready_27 : i1
    %257 = arith.xori %255, %true : i1
    %258 = arith.ori %257, %256 : i1
    %259 = arith.xori %252, %true : i1
    %260 = arith.ori %259, %253 : i1
    %261 = acir.extract %value_26 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc33)
    %262 = arith.andi %ready_27, %242 : i1 loc(#loc33)
    %263 = arith.cmpi eq, %261, %c0_i32_24 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc33)
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
    %274 = arith.select %255, %false_25, %263 : i1
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
    %290 = acir.extract %value_26 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc34)
    %291 = acir.extract %290 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc34)
    %292 = arith.xori %284, %true : i1
    %293 = arith.ori %292, %287 : i1
    %294 = arith.ori %284, %281 : i1
    %295 = arith.xori %281, %true : i1
    %296 = arith.ori %295, %289 : i1
    %297 = arith.andi %293, %296 : i1
    %298 = arith.andi %284, %287 : i1
    %299 = arith.andi %289, %ready_27 : i1
    %300 = arith.andi %281, %299 : i1
    %301 = arith.ori %298, %300 : i1
    %302 = arith.select %284, %false_25, %291 : i1
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
    %318 = acir.extract %value_26 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc35)
    %319 = acir.extract %318 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc35)
    %320 = arith.xori %312, %true : i1
    %321 = arith.ori %320, %315 : i1
    %322 = arith.ori %312, %309 : i1
    %323 = arith.xori %309, %true : i1
    %324 = arith.ori %323, %317 : i1
    %325 = arith.andi %321, %324 : i1
    %326 = arith.andi %312, %315 : i1
    %327 = arith.andi %317, %ready_27 : i1
    %328 = arith.andi %309, %327 : i1
    %329 = arith.ori %326, %328 : i1
    %330 = arith.select %312, %false_25, %319 : i1
    %331 = arith.xori %233, %true : i1
    %332 = arith.ori %331, %304 : i1
    %333 = arith.andi %332, %329 : i1 loc(#loc36)
    %334 = arith.andi %233, %333 : i1 loc(#loc36)
    %335 = arith.andi %237, %334 : i1 loc(#loc37)
    %336 = arith.andi %233, %335 : i1 loc(#loc37)
    %337 = arith.andi %336, %330 : i1 loc(#loc37)
    %338 = arith.xori %233, %true : i1 loc(#loc37)
    %339 = arith.andi %235, %337 : i1 loc(#loc37)
    %340 = arith.ori %338, %339 : i1 loc(#loc37)
    %341 = arith.andi %308, %340 : i1 loc(#loc37)
    %342 = arith.andi %235, %337 : i1 loc(#loc37)
    %343 = arith.xori %233, %true : i1 loc(#loc37)
    %344 = arith.ori %343, %342 : i1 loc(#loc37)
    %345 = arith.andi %233, %342 : i1 loc(#loc37)
    %c0_i32_28 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_29 = arith.constant {acir.source_type = "bool"} false loc(#loc38)
    %value_30, %ready_31 = acir.read(%233) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc39)
    %346 = arith.xori %233, %true : i1 loc(#loc39)
    %347 = arith.andi %344, %ready_31 : i1 loc(#loc39)
    %348 = arith.ori %346, %347 : i1 loc(#loc39)
    %349 = arith.andi %341, %348 : i1 loc(#loc39)
    %350 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %351 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %352 = acir.aggregate(%false_29, %c0_i32_28, %c0_i32_28, %c0_i32_28, %350, %351) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc38)
    %353 = arith.xori %233, %true : i1 loc(#loc40)
    %354 = arith.ori %353, %344 : i1 loc(#loc40)
    %355 = arith.andi %349, %354 : i1 loc(#loc40)
    %356 = arith.andi %344, %ready_31 : i1 loc(#loc41)
    %357 = arith.andi %233, %356 : i1 loc(#loc41)
    %358 = arith.andi %357, %337 : i1 loc(#loc41)
    %359 = arith.xori %188, %true : i1 loc(#loc42)
    %360 = arith.ori %359, %187 : i1 loc(#loc42)
    acir.path %188 {branch = 3 : i64, kind = "branch", txn = 0 : i64} loc(#loc43)
    acir.path %188 {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc43)
    %361 = arith.xori %188, %true : i1 loc(#loc43)
    %362 = arith.ori %361, %360 : i1 loc(#loc43)
    %363 = arith.andi %188, %360 : i1 loc(#loc43)
    %364 = arith.xori %188, %true : i1 loc(#loc43)
    %365 = arith.ori %364, %362 : i1 loc(#loc43)
    %366 = arith.andi %362, %363 : i1 loc(#loc43)
    %367 = arith.andi %188, %366 : i1 loc(#loc43)
    %false_32 = arith.constant {acir.source_type = "bool"} false
    %value_33, %ready_34 = acir.read(%188) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc31)
    %368 = arith.xori %188, %true : i1 loc(#loc31)
    %369 = arith.andi %365, %ready_34 : i1 loc(#loc31)
    %370 = arith.ori %368, %369 : i1 loc(#loc31)
    %371 = arith.andi %355, %370 : i1 loc(#loc31)
    %372 = acir.extract %value_33 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc32)
    %373 = arith.xori %188, %true : i1
    %374 = arith.andi %365, %ready_34 : i1
    %375 = arith.ori %373, %374 : i1
    %376 = arith.andi %371, %375 : i1
    %377 = arith.andi %188, %372 : i1
    %378 = arith.andi %365, %ready_34 : i1
    %379 = arith.xori %372, %true : i1
    %380 = arith.andi %188, %379 : i1
    %381 = arith.andi %365, %ready_34 : i1
    %382 = arith.xori %380, %true : i1
    %383 = arith.ori %382, %381 : i1
    %384 = arith.xori %377, %true : i1
    %385 = arith.ori %384, %378 : i1
    %386 = acir.extract %value_33 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc33)
    %387 = arith.andi %ready_34, %367 : i1 loc(#loc33)
    %388 = arith.cmpi eq, %386, %c0_i32_24 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc33)
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
    %399 = arith.select %380, %false_32, %388 : i1
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
    %415 = acir.extract %value_33 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc34)
    %416 = acir.extract %415 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc34)
    %417 = arith.xori %409, %true : i1
    %418 = arith.ori %417, %412 : i1
    %419 = arith.ori %409, %406 : i1
    %420 = arith.xori %406, %true : i1
    %421 = arith.ori %420, %414 : i1
    %422 = arith.andi %418, %421 : i1
    %423 = arith.andi %409, %412 : i1
    %424 = arith.andi %414, %ready_34 : i1
    %425 = arith.andi %406, %424 : i1
    %426 = arith.ori %423, %425 : i1
    %427 = arith.select %409, %false_32, %416 : i1
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
    %443 = acir.extract %value_33 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc35)
    %444 = acir.extract %443 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc35)
    %445 = arith.xori %437, %true : i1
    %446 = arith.ori %445, %440 : i1
    %447 = arith.ori %437, %434 : i1
    %448 = arith.xori %434, %true : i1
    %449 = arith.ori %448, %442 : i1
    %450 = arith.andi %446, %449 : i1
    %451 = arith.andi %437, %440 : i1
    %452 = arith.andi %442, %ready_34 : i1
    %453 = arith.andi %434, %452 : i1
    %454 = arith.ori %451, %453 : i1
    %455 = arith.select %437, %false_32, %444 : i1
    %456 = arith.xori %188, %true : i1
    %457 = arith.ori %456, %429 : i1
    %458 = arith.andi %457, %454 : i1 loc(#loc36)
    %459 = arith.andi %188, %458 : i1 loc(#loc36)
    %460 = arith.andi %362, %459 : i1 loc(#loc37)
    %461 = arith.andi %188, %460 : i1 loc(#loc37)
    %462 = arith.andi %461, %455 : i1 loc(#loc37)
    %463 = arith.xori %188, %true : i1 loc(#loc37)
    %464 = arith.andi %360, %462 : i1 loc(#loc37)
    %465 = arith.ori %463, %464 : i1 loc(#loc37)
    %466 = arith.andi %433, %465 : i1 loc(#loc37)
    %467 = arith.andi %360, %462 : i1 loc(#loc37)
    %468 = arith.xori %188, %true : i1 loc(#loc37)
    %469 = arith.ori %468, %467 : i1 loc(#loc37)
    %470 = arith.andi %188, %467 : i1 loc(#loc37)
    %c0_i32_35 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_36 = arith.constant {acir.source_type = "bool"} false loc(#loc38)
    %value_37, %ready_38 = acir.read(%188) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc39)
    %471 = arith.xori %188, %true : i1 loc(#loc39)
    %472 = arith.andi %469, %ready_38 : i1 loc(#loc39)
    %473 = arith.ori %471, %472 : i1 loc(#loc39)
    %474 = arith.andi %466, %473 : i1 loc(#loc39)
    %475 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %476 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %477 = acir.aggregate(%false_36, %c0_i32_35, %c0_i32_35, %c0_i32_35, %475, %476) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc38)
    %478 = arith.xori %188, %true : i1 loc(#loc40)
    %479 = arith.ori %478, %469 : i1 loc(#loc40)
    %480 = arith.andi %474, %479 : i1 loc(#loc40)
    %481 = arith.andi %469, %ready_38 : i1 loc(#loc41)
    %482 = arith.andi %188, %481 : i1 loc(#loc41)
    %483 = arith.andi %482, %462 : i1 loc(#loc41)
    %484 = arith.andi %235, %358 : i1 loc(#loc44)
    %485 = arith.andi %233, %484 : i1 loc(#loc44)
    %486 = arith.andi %360, %483 : i1 loc(#loc44)
    %487 = arith.andi %188, %486 : i1 loc(#loc44)
    %488 = arith.ori %485, %487 : i1 loc(#loc44)
    %489 = arith.select %233, %value_30, %value_37 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc44)
    %490 = arith.xori %488, %true : i1 loc(#loc44)
    acir.path %488 {branch = 4 : i64, kind = "branch", txn = 0 : i64} loc(#loc44)
    %value_39, %ready_40 = acir.read(%488) {branch = 4 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.scheduler.add_requests", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc44)
    %491 = arith.xori %488, %true : i1 loc(#loc44)
    %492 = arith.ori %491, %ready_40 : i1 loc(#loc44)
    %493 = arith.andi %480, %492 : i1 loc(#loc44)
    %494 = arith.xori %value_39, %true : i1 loc(#loc44)
    %495 = arith.xori %488, %true : i1 loc(#loc44)
    %496 = arith.ori %495, %494 : i1 loc(#loc44)
    %497 = arith.andi %493, %496 : i1 loc(#loc44)
    %498 = acir.fire "top.scheduler.issue_add#0" %497 {successors = [], txn = 0 : i64} loc(#loc44)
    %499 = arith.andi %498, %233 : i1 loc(#loc44)
    acir.effect %499(%352) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc40)
    %500 = arith.andi %498, %188 : i1 loc(#loc44)
    acir.effect %500(%477) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc40)
    %501 = arith.andi %498, %488 : i1 loc(#loc44)
    acir.effect %501(%489) {branch = 4 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.scheduler.add_requests", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc44)
    acir.path %true {branch = 5 : i64, kind = "branch", txn = 1 : i64} loc(#loc45)
    acir.path %true {branch = 5 : i64, frames = ["top.scheduler:issue_mul"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc45)
    %c1_i32_41 = arith.constant {acir.source_type = "u32"} 1 : i32
    %502 = arith.andi %230, %230 : i1
    %503 = arith.xori %231, %true : i1
    %504 = arith.xori %503, %true : i1
    %505 = arith.ori %504, %230 : i1
    acir.path %503 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc46)
    acir.path %503 {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc46)
    %506 = arith.xori %503, %true : i1 loc(#loc46)
    %507 = arith.ori %506, %505 : i1 loc(#loc46)
    %508 = arith.andi %503, %505 : i1 loc(#loc46)
    %509 = arith.xori %503, %true : i1 loc(#loc46)
    %510 = arith.ori %509, %507 : i1 loc(#loc46)
    %511 = arith.andi %507, %508 : i1 loc(#loc46)
    %512 = arith.andi %503, %511 : i1 loc(#loc46)
    %false_42 = arith.constant {acir.source_type = "bool"} false
    %value_43, %ready_44 = acir.read(%503) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc31)
    %513 = arith.xori %503, %true : i1 loc(#loc31)
    %514 = arith.andi %510, %ready_44 : i1 loc(#loc31)
    %515 = arith.ori %513, %514 : i1 loc(#loc31)
    %516 = arith.andi %502, %515 : i1 loc(#loc31)
    %517 = acir.extract %value_43 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc32)
    %518 = arith.xori %503, %true : i1
    %519 = arith.andi %510, %ready_44 : i1
    %520 = arith.ori %518, %519 : i1
    %521 = arith.andi %516, %520 : i1
    %522 = arith.andi %503, %517 : i1
    %523 = arith.andi %510, %ready_44 : i1
    %524 = arith.xori %517, %true : i1
    %525 = arith.andi %503, %524 : i1
    %526 = arith.andi %510, %ready_44 : i1
    %527 = arith.xori %525, %true : i1
    %528 = arith.ori %527, %526 : i1
    %529 = arith.xori %522, %true : i1
    %530 = arith.ori %529, %523 : i1
    %531 = acir.extract %value_43 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc33)
    %532 = arith.andi %ready_44, %512 : i1 loc(#loc33)
    %533 = arith.cmpi eq, %531, %c1_i32_41 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc33)
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
    %544 = arith.select %525, %false_42, %533 : i1
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
    %560 = acir.extract %value_43 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc34)
    %561 = acir.extract %560 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc34)
    %562 = arith.xori %554, %true : i1
    %563 = arith.ori %562, %557 : i1
    %564 = arith.ori %554, %551 : i1
    %565 = arith.xori %551, %true : i1
    %566 = arith.ori %565, %559 : i1
    %567 = arith.andi %563, %566 : i1
    %568 = arith.andi %554, %557 : i1
    %569 = arith.andi %559, %ready_44 : i1
    %570 = arith.andi %551, %569 : i1
    %571 = arith.ori %568, %570 : i1
    %572 = arith.select %554, %false_42, %561 : i1
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
    %588 = acir.extract %value_43 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc35)
    %589 = acir.extract %588 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc35)
    %590 = arith.xori %582, %true : i1
    %591 = arith.ori %590, %585 : i1
    %592 = arith.ori %582, %579 : i1
    %593 = arith.xori %579, %true : i1
    %594 = arith.ori %593, %587 : i1
    %595 = arith.andi %591, %594 : i1
    %596 = arith.andi %582, %585 : i1
    %597 = arith.andi %587, %ready_44 : i1
    %598 = arith.andi %579, %597 : i1
    %599 = arith.ori %596, %598 : i1
    %600 = arith.select %582, %false_42, %589 : i1
    %601 = arith.xori %503, %true : i1
    %602 = arith.ori %601, %574 : i1
    %603 = arith.andi %602, %599 : i1 loc(#loc36)
    %604 = arith.andi %503, %603 : i1 loc(#loc36)
    %605 = arith.andi %507, %604 : i1 loc(#loc37)
    %606 = arith.andi %503, %605 : i1 loc(#loc37)
    %607 = arith.andi %606, %600 : i1 loc(#loc37)
    %608 = arith.xori %503, %true : i1 loc(#loc37)
    %609 = arith.andi %505, %607 : i1 loc(#loc37)
    %610 = arith.ori %608, %609 : i1 loc(#loc37)
    %611 = arith.andi %578, %610 : i1 loc(#loc37)
    %612 = arith.andi %505, %607 : i1 loc(#loc37)
    %613 = arith.xori %503, %true : i1 loc(#loc37)
    %614 = arith.ori %613, %612 : i1 loc(#loc37)
    %615 = arith.andi %503, %612 : i1 loc(#loc37)
    %c0_i32_45 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_46 = arith.constant {acir.source_type = "bool"} false loc(#loc38)
    %value_47, %ready_48 = acir.read(%503) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc39)
    %616 = arith.xori %503, %true : i1 loc(#loc39)
    %617 = arith.andi %614, %ready_48 : i1 loc(#loc39)
    %618 = arith.ori %616, %617 : i1 loc(#loc39)
    %619 = arith.andi %611, %618 : i1 loc(#loc39)
    %620 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %621 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %622 = acir.aggregate(%false_46, %c0_i32_45, %c0_i32_45, %c0_i32_45, %620, %621) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc38)
    %623 = arith.xori %503, %true : i1 loc(#loc40)
    %624 = arith.ori %623, %614 : i1 loc(#loc40)
    %625 = arith.andi %619, %624 : i1 loc(#loc40)
    %626 = arith.andi %614, %ready_48 : i1 loc(#loc41)
    %627 = arith.andi %503, %626 : i1 loc(#loc41)
    %628 = arith.andi %627, %607 : i1 loc(#loc41)
    %629 = arith.xori %231, %true : i1 loc(#loc47)
    %630 = arith.ori %629, %230 : i1 loc(#loc47)
    acir.path %231 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc48)
    acir.path %231 {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc48)
    %631 = arith.xori %231, %true : i1 loc(#loc48)
    %632 = arith.ori %631, %630 : i1 loc(#loc48)
    %633 = arith.andi %231, %630 : i1 loc(#loc48)
    %634 = arith.xori %231, %true : i1 loc(#loc48)
    %635 = arith.ori %634, %632 : i1 loc(#loc48)
    %636 = arith.andi %632, %633 : i1 loc(#loc48)
    %637 = arith.andi %231, %636 : i1 loc(#loc48)
    %false_49 = arith.constant {acir.source_type = "bool"} false
    %value_50, %ready_51 = acir.read(%231) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc31)
    %638 = arith.xori %231, %true : i1 loc(#loc31)
    %639 = arith.andi %635, %ready_51 : i1 loc(#loc31)
    %640 = arith.ori %638, %639 : i1 loc(#loc31)
    %641 = arith.andi %625, %640 : i1 loc(#loc31)
    %642 = acir.extract %value_50 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc32)
    %643 = arith.xori %231, %true : i1
    %644 = arith.andi %635, %ready_51 : i1
    %645 = arith.ori %643, %644 : i1
    %646 = arith.andi %641, %645 : i1
    %647 = arith.andi %231, %642 : i1
    %648 = arith.andi %635, %ready_51 : i1
    %649 = arith.xori %642, %true : i1
    %650 = arith.andi %231, %649 : i1
    %651 = arith.andi %635, %ready_51 : i1
    %652 = arith.xori %650, %true : i1
    %653 = arith.ori %652, %651 : i1
    %654 = arith.xori %647, %true : i1
    %655 = arith.ori %654, %648 : i1
    %656 = acir.extract %value_50 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc33)
    %657 = arith.andi %ready_51, %637 : i1 loc(#loc33)
    %658 = arith.cmpi eq, %656, %c1_i32_41 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc33)
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
    %669 = arith.select %650, %false_49, %658 : i1
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
    %685 = acir.extract %value_50 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc34)
    %686 = acir.extract %685 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc34)
    %687 = arith.xori %679, %true : i1
    %688 = arith.ori %687, %682 : i1
    %689 = arith.ori %679, %676 : i1
    %690 = arith.xori %676, %true : i1
    %691 = arith.ori %690, %684 : i1
    %692 = arith.andi %688, %691 : i1
    %693 = arith.andi %679, %682 : i1
    %694 = arith.andi %684, %ready_51 : i1
    %695 = arith.andi %676, %694 : i1
    %696 = arith.ori %693, %695 : i1
    %697 = arith.select %679, %false_49, %686 : i1
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
    %713 = acir.extract %value_50 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc35)
    %714 = acir.extract %713 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc35)
    %715 = arith.xori %707, %true : i1
    %716 = arith.ori %715, %710 : i1
    %717 = arith.ori %707, %704 : i1
    %718 = arith.xori %704, %true : i1
    %719 = arith.ori %718, %712 : i1
    %720 = arith.andi %716, %719 : i1
    %721 = arith.andi %707, %710 : i1
    %722 = arith.andi %712, %ready_51 : i1
    %723 = arith.andi %704, %722 : i1
    %724 = arith.ori %721, %723 : i1
    %725 = arith.select %707, %false_49, %714 : i1
    %726 = arith.xori %231, %true : i1
    %727 = arith.ori %726, %699 : i1
    %728 = arith.andi %727, %724 : i1 loc(#loc36)
    %729 = arith.andi %231, %728 : i1 loc(#loc36)
    %730 = arith.andi %632, %729 : i1 loc(#loc37)
    %731 = arith.andi %231, %730 : i1 loc(#loc37)
    %732 = arith.andi %731, %725 : i1 loc(#loc37)
    %733 = arith.xori %231, %true : i1 loc(#loc37)
    %734 = arith.andi %630, %732 : i1 loc(#loc37)
    %735 = arith.ori %733, %734 : i1 loc(#loc37)
    %736 = arith.andi %703, %735 : i1 loc(#loc37)
    %737 = arith.andi %630, %732 : i1 loc(#loc37)
    %738 = arith.xori %231, %true : i1 loc(#loc37)
    %739 = arith.ori %738, %737 : i1 loc(#loc37)
    %740 = arith.andi %231, %737 : i1 loc(#loc37)
    %c0_i32_52 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_53 = arith.constant {acir.source_type = "bool"} false loc(#loc38)
    %value_54, %ready_55 = acir.read(%231) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc39)
    %741 = arith.xori %231, %true : i1 loc(#loc39)
    %742 = arith.andi %739, %ready_55 : i1 loc(#loc39)
    %743 = arith.ori %741, %742 : i1 loc(#loc39)
    %744 = arith.andi %736, %743 : i1 loc(#loc39)
    %745 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %746 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc38)
    %747 = acir.aggregate(%false_53, %c0_i32_52, %c0_i32_52, %c0_i32_52, %745, %746) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc38)
    %748 = arith.xori %231, %true : i1 loc(#loc40)
    %749 = arith.ori %748, %739 : i1 loc(#loc40)
    %750 = arith.andi %744, %749 : i1 loc(#loc40)
    %751 = arith.andi %739, %ready_55 : i1 loc(#loc41)
    %752 = arith.andi %231, %751 : i1 loc(#loc41)
    %753 = arith.andi %752, %732 : i1 loc(#loc41)
    %754 = arith.andi %505, %628 : i1 loc(#loc49)
    %755 = arith.andi %503, %754 : i1 loc(#loc49)
    %756 = arith.andi %630, %753 : i1 loc(#loc49)
    %757 = arith.andi %231, %756 : i1 loc(#loc49)
    %758 = arith.ori %755, %757 : i1 loc(#loc49)
    %759 = arith.select %503, %value_47, %value_54 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc49)
    %760 = arith.xori %758, %true : i1 loc(#loc49)
    acir.path %758 {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc49)
    %value_56, %ready_57 = acir.read(%758) {branch = 8 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.scheduler.mul_requests", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc49)
    %761 = arith.xori %758, %true : i1 loc(#loc49)
    %762 = arith.ori %761, %ready_57 : i1 loc(#loc49)
    %763 = arith.andi %750, %762 : i1 loc(#loc49)
    %764 = arith.xori %value_56, %true : i1 loc(#loc49)
    %765 = arith.xori %758, %true : i1 loc(#loc49)
    %766 = arith.ori %765, %764 : i1 loc(#loc49)
    %767 = arith.andi %763, %766 : i1 loc(#loc49)
    %768 = acir.fire "top.scheduler.issue_mul#1" %767 {successors = [], txn = 1 : i64} loc(#loc49)
    %769 = arith.andi %768, %503 : i1 loc(#loc49)
    acir.effect %769(%622) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc40)
    %770 = arith.andi %768, %231 : i1 loc(#loc49)
    acir.effect %770(%747) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc40)
    %771 = arith.andi %768, %758 : i1 loc(#loc49)
    acir.effect %771(%759) {branch = 8 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.scheduler.mul_requests", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc49)
    acir.path %true {branch = 9 : i64, kind = "branch", txn = 2 : i64} loc(#loc50)
    acir.path %true {branch = 9 : i64, frames = ["top.adder:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc50)
    %value_58, %ready_59 = acir.read(%true) {branch = 9 : i64, frames = ["top.adder:execute"], method = "peek", port = 0 : i64, resource = "top.scheduler.add_requests", txn = 2 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc51)
    %772 = arith.andi %ready_59, %ready_59 : i1 loc(#loc51)
    %value_60, %ready_61 = acir.read(%true) {branch = 9 : i64, frames = ["top.adder:execute"], method = "empty", port = 0 : i64, resource = "top.scheduler.add_requests", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc51)
    %773 = arith.andi %772, %ready_61 : i1 loc(#loc51)
    %774 = arith.xori %value_60, %true : i1 loc(#loc51)
    %775 = arith.andi %773, %774 : i1 loc(#loc51)
    %776 = acir.extract %value_58 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc52)
    %777 = acir.extract %value_58 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc53)
    %778 = acir.extract %value_58 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc54)
    %779 = acir.extract %778 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc54)
    %780 = acir.extract %value_58 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc55)
    %781 = acir.extract %780 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc55)
    %782 = arith.andi %ready_59, %ready_59 : i1 loc(#loc54)
    %783 = arith.addi %779, %781 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc54)
    %784 = arith.andi %ready_59, %ready_59 : i1 loc(#loc56)
    %785 = arith.andi %784, %782 : i1 loc(#loc56)
    %786 = acir.aggregate(%776, %777, %783) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc56)
    %787 = arith.xori %785, %true : i1 loc(#loc57)
    acir.path %785 {branch = 10 : i64, kind = "branch", txn = 2 : i64} loc(#loc57)
    %value_62, %ready_63 = acir.read(%785) {branch = 10 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.adder.out", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc57)
    %788 = arith.xori %785, %true : i1 loc(#loc57)
    %789 = arith.ori %788, %ready_63 : i1 loc(#loc57)
    %790 = arith.andi %775, %789 : i1 loc(#loc57)
    %791 = arith.xori %value_62, %true : i1 loc(#loc57)
    %792 = arith.xori %785, %true : i1 loc(#loc57)
    %793 = arith.ori %792, %791 : i1 loc(#loc57)
    %794 = arith.andi %790, %793 : i1 loc(#loc57)
    %795 = acir.fire "top.adder.execute#2" %794 {successors = [0], txn = 2 : i64} loc(#loc57)
    acir.effect %795() {branch = 9 : i64, frames = ["top.adder:execute"], method = "pop", port = 0 : i64, resource = "top.scheduler.add_requests", txn = 2 : i64} : (i1) -> () loc(#loc51)
    %796 = arith.andi %795, %785 : i1 loc(#loc57)
    acir.effect %796(%786) {branch = 10 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.adder.out", txn = 2 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> () loc(#loc57)
    %value_64, %ready_65 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.multiplier.active", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc58)
    %797 = arith.xori %value_64, %true : i1
    %798 = arith.xori %797, %true : i1
    %799 = arith.ori %798, %ready_65 : i1
    %800 = arith.andi %797, %799 : i1 loc(#loc59)
    acir.path %797 {branch = 11 : i64, kind = "branch", txn = 3 : i64} loc(#loc59)
    acir.path %797 {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc59)
    %801 = arith.xori %797, %true : i1 loc(#loc59)
    %802 = arith.ori %801, %799 : i1 loc(#loc59)
    %803 = arith.andi %797, %799 : i1 loc(#loc59)
    %true_66 = arith.constant {acir.source_type = "bool"} true loc(#loc60)
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc61)
    %value_67, %ready_68 = acir.read(%797) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "peek", port = 0 : i64, resource = "top.scheduler.mul_requests", txn = 3 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc62)
    %804 = arith.xori %797, %true : i1 loc(#loc62)
    %805 = arith.andi %802, %ready_68 : i1 loc(#loc62)
    %806 = arith.ori %804, %805 : i1 loc(#loc62)
    %807 = arith.andi %800, %806 : i1 loc(#loc62)
    %808 = arith.xori %797, %true : i1 loc(#loc62)
    %809 = arith.andi %802, %ready_68 : i1 loc(#loc62)
    %810 = arith.ori %808, %809 : i1 loc(#loc62)
    %811 = arith.andi %807, %810 : i1 loc(#loc62)
    %value_69, %ready_70 = acir.read(%797) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "empty", port = 0 : i64, resource = "top.scheduler.mul_requests", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc62)
    %812 = arith.xori %797, %true : i1 loc(#loc62)
    %813 = arith.andi %802, %ready_70 : i1 loc(#loc62)
    %814 = arith.ori %812, %813 : i1 loc(#loc62)
    %815 = arith.andi %811, %814 : i1 loc(#loc62)
    %816 = arith.xori %value_69, %true : i1 loc(#loc62)
    %817 = arith.xori %797, %true : i1 loc(#loc62)
    %818 = arith.andi %802, %816 : i1 loc(#loc62)
    %819 = arith.ori %817, %818 : i1 loc(#loc62)
    %820 = arith.andi %815, %819 : i1 loc(#loc62)
    %821 = arith.xori %797, %true : i1 loc(#loc63)
    %822 = arith.andi %802, %ready_68 : i1 loc(#loc63)
    %823 = arith.ori %821, %822 : i1 loc(#loc63)
    %824 = arith.andi %820, %823 : i1 loc(#loc63)
    %825 = arith.xori %797, %true : i1 loc(#loc64)
    %826 = arith.ori %825, %802 : i1 loc(#loc64)
    %827 = arith.andi %824, %826 : i1 loc(#loc64)
    %828 = arith.xori %797, %true : i1 loc(#loc65)
    %829 = arith.ori %828, %802 : i1 loc(#loc65)
    %830 = arith.andi %827, %829 : i1 loc(#loc65)
    %831 = arith.andi %797, %802 : i1
    %832 = acir.fire "top.multiplier.accept#3" %830 {successors = [1], txn = 3 : i64}
    %833 = arith.andi %832, %797 : i1
    acir.effect %833() {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "pop", port = 0 : i64, resource = "top.scheduler.mul_requests", txn = 3 : i64} : (i1) -> () loc(#loc62)
    %834 = arith.andi %832, %797 : i1
    acir.effect %834(%value_67) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.request", txn = 3 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc63)
    %835 = arith.andi %832, %797 : i1
    acir.effect %835(%c2_i32) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 3 : i64} : (i1, i32) -> () loc(#loc64)
    %836 = arith.andi %832, %797 : i1
    acir.effect %836(%true_66) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 3 : i64} : (i1, i1) -> () loc(#loc65)
    %837 = arith.xori %value_64, %true : i1
    %838 = arith.ori %837, %ready_65 : i1
    %839 = arith.andi %value_64, %838 : i1 loc(#loc66)
    acir.path %value_64 {branch = 12 : i64, kind = "branch", txn = 4 : i64} loc(#loc66)
    acir.path %value_64 {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc66)
    %840 = arith.xori %value_64, %true : i1 loc(#loc66)
    %841 = arith.ori %840, %838 : i1 loc(#loc66)
    %false_71 = arith.constant {acir.source_type = "bool"} false
    %c1_i32_72 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32_73 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc67)
    %value_74, %ready_75 = acir.read(%value_64) {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc68)
    %842 = arith.xori %value_64, %true : i1 loc(#loc68)
    %843 = arith.andi %841, %ready_75 : i1 loc(#loc68)
    %844 = arith.ori %842, %843 : i1 loc(#loc68)
    %845 = arith.andi %839, %844 : i1 loc(#loc68)
    %846 = arith.cmpi ne, %value_74, %c0_i32_73 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc68)
    %847 = arith.xori %value_64, %true : i1
    %848 = arith.andi %841, %ready_75 : i1
    %849 = arith.ori %847, %848 : i1
    %850 = arith.andi %845, %849 : i1
    %851 = arith.andi %value_64, %846 : i1
    %852 = arith.andi %841, %ready_75 : i1
    %853 = arith.xori %846, %true : i1
    %854 = arith.andi %value_64, %853 : i1
    %855 = arith.andi %841, %ready_75 : i1
    %856 = arith.xori %854, %true : i1
    %857 = arith.ori %856, %855 : i1
    acir.path %854 {branch = 13 : i64, kind = "branch", txn = 4 : i64} loc(#loc69)
    %value_76, %ready_77 = acir.read(%854) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.request", txn = 4 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc69)
    %858 = arith.xori %854, %true : i1 loc(#loc69)
    %859 = arith.andi %857, %ready_77 : i1 loc(#loc69)
    %860 = arith.ori %858, %859 : i1 loc(#loc69)
    %861 = arith.andi %850, %860 : i1 loc(#loc69)
    %862 = arith.xori %854, %true : i1 loc(#loc70)
    %863 = arith.ori %862, %857 : i1 loc(#loc70)
    %864 = arith.andi %861, %863 : i1 loc(#loc70)
    %865 = acir.extract %value_76 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc71)
    %866 = acir.extract %value_76 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc72)
    %867 = acir.extract %value_76 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc73)
    %868 = acir.extract %867 {acir.id = "v25", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc73)
    %869 = acir.extract %value_76 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc74)
    %870 = acir.extract %869 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc74)
    %871 = arith.andi %ready_77, %ready_77 : i1 loc(#loc73)
    %872 = arith.muli %868, %870 {acir.id = "v28", acir.source_type = "u32"} : i32 loc(#loc73)
    %873 = arith.andi %ready_77, %ready_77 : i1 loc(#loc75)
    %874 = arith.andi %873, %871 : i1 loc(#loc75)
    %875 = acir.aggregate(%865, %866, %872) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc75)
    %876 = arith.xori %851, %true : i1 loc(#loc76)
    %877 = arith.ori %876, %852 : i1 loc(#loc76)
    acir.path %851 {branch = 14 : i64, kind = "branch", txn = 4 : i64} loc(#loc77)
    %value_78, %ready_79 = acir.read(%851) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc77)
    %878 = arith.xori %851, %true : i1 loc(#loc77)
    %879 = arith.andi %877, %ready_79 : i1 loc(#loc77)
    %880 = arith.ori %878, %879 : i1 loc(#loc77)
    %881 = arith.andi %864, %880 : i1 loc(#loc77)
    %882 = arith.subi %value_78, %c1_i32_72 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc77)
    %883 = arith.xori %851, %true : i1 loc(#loc78)
    %884 = arith.andi %877, %ready_79 : i1 loc(#loc78)
    %885 = arith.ori %883, %884 : i1 loc(#loc78)
    %886 = arith.andi %881, %885 : i1 loc(#loc78)
    %887 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc79)
    %888 = arith.andi %857, %874 : i1 loc(#loc79)
    %889 = arith.andi %854, %888 : i1 loc(#loc79)
    %890 = arith.select %854, %875, %887 : !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc79)
    %891 = arith.andi %value_64, %889 : i1 loc(#loc79)
    %892 = arith.xori %891, %true : i1 loc(#loc79)
    %893 = arith.ori %892, %838 : i1 loc(#loc79)
    %894 = arith.andi %886, %893 : i1 loc(#loc79)
    acir.path %891 {branch = 15 : i64, kind = "branch", txn = 4 : i64} loc(#loc79)
    %value_80, %ready_81 = acir.read(%891) {branch = 15 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.multiplier.out", txn = 4 : i64} : (i1) -> (i1, i1) loc(#loc79)
    %895 = arith.xori %891, %true : i1 loc(#loc79)
    %896 = arith.andi %838, %ready_81 : i1 loc(#loc79)
    %897 = arith.ori %895, %896 : i1 loc(#loc79)
    %898 = arith.andi %894, %897 : i1 loc(#loc79)
    %899 = arith.xori %value_80, %true : i1 loc(#loc79)
    %900 = arith.xori %891, %true : i1 loc(#loc79)
    %901 = arith.andi %838, %899 : i1 loc(#loc79)
    %902 = arith.ori %900, %901 : i1 loc(#loc79)
    %903 = arith.andi %898, %902 : i1 loc(#loc79)
    %904 = acir.fire "top.multiplier.advance#4" %903 {successors = [], txn = 4 : i64} loc(#loc79)
    %905 = arith.andi %904, %854 : i1 loc(#loc79)
    acir.effect %905(%false_71) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 4 : i64} : (i1, i1) -> () loc(#loc70)
    %906 = arith.andi %904, %851 : i1 loc(#loc79)
    acir.effect %906(%882) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1, i32) -> () loc(#loc78)
    %907 = arith.andi %904, %891 : i1 loc(#loc79)
    acir.effect %907(%890) {branch = 15 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.multiplier.out", txn = 4 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> () loc(#loc79)
    %908 = arith.xori %797, %true : i1
    %909 = arith.ori %908, %799 : i1
    %910 = arith.ori %797, %value_64 : i1
    %911 = arith.xori %value_64, %true : i1
    %912 = arith.ori %911, %838 : i1
    %913 = arith.andi %909, %912 : i1
    %value_82, %ready_83 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.adder.out", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc80)
    %914 = acir.unary %value_82 "not" {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1 loc(#loc81)
    acir.path %true {branch = 16 : i64, kind = "branch", txn = 5 : i64} loc(#loc82)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc82)
    %915 = arith.andi %ready_83, %ready_83 : i1
    %916 = arith.xori %914, %true : i1
    %917 = arith.xori %916, %true : i1
    %918 = arith.ori %917, %ready_83 : i1
    acir.path %916 {branch = 17 : i64, kind = "branch", txn = 5 : i64} loc(#loc83)
    %value_84, %ready_85 = acir.read(%916) {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.multiplier.out", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc83)
    %919 = arith.xori %916, %true : i1 loc(#loc83)
    %920 = arith.andi %918, %ready_85 : i1 loc(#loc83)
    %921 = arith.ori %919, %920 : i1 loc(#loc83)
    %922 = arith.andi %915, %921 : i1 loc(#loc83)
    %923 = arith.xori %916, %true : i1 loc(#loc83)
    %924 = arith.andi %918, %ready_85 : i1 loc(#loc83)
    %925 = arith.ori %923, %924 : i1 loc(#loc83)
    %926 = arith.andi %922, %925 : i1 loc(#loc83)
    %value_86, %ready_87 = acir.read(%916) {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "empty", port = 0 : i64, resource = "top.multiplier.out", txn = 5 : i64} : (i1) -> (i1, i1) loc(#loc83)
    %927 = arith.xori %916, %true : i1 loc(#loc83)
    %928 = arith.andi %918, %ready_87 : i1 loc(#loc83)
    %929 = arith.ori %927, %928 : i1 loc(#loc83)
    %930 = arith.andi %926, %929 : i1 loc(#loc83)
    %931 = arith.xori %value_86, %true : i1 loc(#loc83)
    %932 = arith.xori %916, %true : i1 loc(#loc83)
    %933 = arith.andi %918, %931 : i1 loc(#loc83)
    %934 = arith.ori %932, %933 : i1 loc(#loc83)
    %935 = arith.andi %930, %934 : i1 loc(#loc83)
    %936 = arith.xori %914, %true : i1
    %937 = arith.ori %936, %ready_83 : i1
    acir.path %914 {branch = 18 : i64, kind = "branch", txn = 5 : i64} loc(#loc84)
    %value_88, %ready_89 = acir.read(%914) {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.adder.out", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc84)
    %938 = arith.xori %914, %true : i1 loc(#loc84)
    %939 = arith.andi %937, %ready_89 : i1 loc(#loc84)
    %940 = arith.ori %938, %939 : i1 loc(#loc84)
    %941 = arith.andi %935, %940 : i1 loc(#loc84)
    %942 = arith.xori %914, %true : i1 loc(#loc84)
    %943 = arith.andi %937, %ready_89 : i1 loc(#loc84)
    %944 = arith.ori %942, %943 : i1 loc(#loc84)
    %945 = arith.andi %941, %944 : i1 loc(#loc84)
    %value_90, %ready_91 = acir.read(%914) {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "empty", port = 0 : i64, resource = "top.adder.out", txn = 5 : i64} : (i1) -> (i1, i1) loc(#loc84)
    %946 = arith.xori %914, %true : i1 loc(#loc84)
    %947 = arith.andi %937, %ready_91 : i1 loc(#loc84)
    %948 = arith.ori %946, %947 : i1 loc(#loc84)
    %949 = arith.andi %945, %948 : i1 loc(#loc84)
    %950 = arith.xori %value_90, %true : i1 loc(#loc84)
    %951 = arith.xori %914, %true : i1 loc(#loc84)
    %952 = arith.andi %937, %950 : i1 loc(#loc84)
    %953 = arith.ori %951, %952 : i1 loc(#loc84)
    %954 = arith.andi %949, %953 : i1 loc(#loc84)
    %955 = arith.xori %916, %true : i1
    %956 = arith.ori %955, %918 : i1
    %957 = arith.ori %916, %914 : i1
    %958 = arith.xori %914, %true : i1
    %959 = arith.ori %958, %937 : i1
    %960 = arith.andi %956, %959 : i1
    %961 = arith.andi %918, %ready_85 : i1
    %962 = arith.andi %916, %961 : i1
    %963 = arith.andi %937, %ready_89 : i1
    %964 = arith.andi %914, %963 : i1
    %965 = arith.ori %962, %964 : i1
    %966 = arith.select %916, %value_84, %value_88 : !acir.struct<"src_payloads_25ebcaa7.Completion">
    %967 = acir.extract %966 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc85)
    %968 = acir.extract %966 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc86)
    %969 = arith.andi %965, %965 : i1 loc(#loc87)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.rob:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc87)
    %c2_i32_92 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_93 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc88)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc89)
    %c1_i32_94 = arith.constant {acir.source_type = "u32"} 1 : i32
    %970 = arith.subi %967, %c1_i32_94 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc90)
    %971 = arith.andi %970, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc90)
    %972 = arith.cmpi eq, %971, %c0_i32_93 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc91)
    %973 = arith.andi %954, %965 : i1
    %974 = arith.xori %972, %true : i1
    %975 = arith.xori %974, %true : i1
    %976 = arith.ori %975, %965 : i1
    %977 = arith.cmpi eq, %971, %c1_i32_94 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc92)
    %978 = arith.xori %974, %true : i1
    %979 = arith.andi %976, %965 : i1
    %980 = arith.ori %978, %979 : i1
    %981 = arith.andi %973, %980 : i1
    %982 = arith.andi %974, %977 : i1
    %983 = arith.andi %976, %965 : i1
    %984 = arith.xori %977, %true : i1
    %985 = arith.andi %974, %984 : i1
    %986 = arith.andi %976, %965 : i1
    %987 = arith.xori %985, %true : i1
    %988 = arith.ori %987, %986 : i1
    %989 = arith.cmpi eq, %971, %c2_i32_92 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc93)
    %990 = arith.xori %985, %true : i1
    %991 = arith.andi %988, %965 : i1
    %992 = arith.ori %990, %991 : i1
    %993 = arith.andi %981, %992 : i1
    %994 = arith.andi %985, %989 : i1
    %995 = arith.andi %988, %965 : i1
    %996 = arith.xori %989, %true : i1
    %997 = arith.andi %985, %996 : i1
    %998 = arith.andi %988, %965 : i1
    %999 = arith.xori %997, %true : i1
    %1000 = arith.ori %999, %998 : i1
    %1001 = arith.andi %965, %965 : i1 loc(#loc94)
    acir.path %997 {branch = 19 : i64, kind = "branch", txn = 5 : i64} loc(#loc94)
    acir.path %997 {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc94)
    %1002 = arith.xori %997, %true : i1 loc(#loc94)
    %1003 = arith.ori %1002, %1000 : i1 loc(#loc94)
    %1004 = arith.andi %1000, %965 : i1 loc(#loc94)
    %1005 = arith.andi %997, %1004 : i1 loc(#loc94)
    %1006 = arith.andi %1000, %965 : i1 loc(#loc94)
    %1007 = arith.andi %997, %1006 : i1 loc(#loc94)
    %true_95 = arith.constant {acir.source_type = "bool"} true
    %false_96 = arith.constant {acir.source_type = "bool"} false
    %value_97, %ready_98 = acir.read(%997) {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc95)
    %1008 = arith.xori %997, %true : i1 loc(#loc95)
    %1009 = arith.andi %1003, %ready_98 : i1 loc(#loc95)
    %1010 = arith.ori %1008, %1009 : i1 loc(#loc95)
    %1011 = arith.andi %993, %1010 : i1 loc(#loc95)
    %1012 = acir.extract %value_97 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc96)
    %1013 = arith.xori %997, %true : i1
    %1014 = arith.andi %1003, %ready_98 : i1
    %1015 = arith.ori %1013, %1014 : i1
    %1016 = arith.andi %1011, %1015 : i1
    %1017 = arith.andi %997, %1012 : i1
    %1018 = arith.andi %1003, %ready_98 : i1
    %1019 = arith.xori %1012, %true : i1
    %1020 = arith.andi %997, %1019 : i1
    %1021 = arith.andi %1003, %ready_98 : i1
    %1022 = arith.xori %1020, %true : i1
    %1023 = arith.ori %1022, %1021 : i1
    %1024 = arith.xori %1017, %true : i1
    %1025 = arith.ori %1024, %1018 : i1
    %1026 = acir.extract %value_97 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc97)
    %1027 = acir.unary %1026 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc98)
    %1028 = arith.xori %1020, %true : i1
    %1029 = arith.ori %1028, %1023 : i1
    %1030 = arith.ori %1020, %1017 : i1
    %1031 = arith.xori %1017, %true : i1
    %1032 = arith.ori %1031, %1025 : i1
    %1033 = arith.andi %1029, %1032 : i1
    %1034 = arith.andi %1020, %1023 : i1
    %1035 = arith.andi %1025, %ready_98 : i1
    %1036 = arith.andi %1017, %1035 : i1
    %1037 = arith.ori %1034, %1036 : i1
    %1038 = arith.select %1020, %false_96, %1027 : i1
    %1039 = arith.xori %997, %true : i1
    %1040 = arith.ori %1039, %1003 : i1
    %1041 = arith.xori %997, %true : i1
    %1042 = arith.andi %1040, %1037 : i1
    %1043 = arith.ori %1041, %1042 : i1
    %1044 = arith.andi %1016, %1043 : i1
    %1045 = arith.andi %997, %1038 : i1
    %1046 = arith.andi %1040, %1037 : i1
    %1047 = arith.xori %1038, %true : i1
    %1048 = arith.andi %997, %1047 : i1
    %1049 = arith.andi %1040, %1037 : i1
    %1050 = arith.xori %1048, %true : i1
    %1051 = arith.ori %1050, %1049 : i1
    %1052 = arith.xori %1045, %true : i1
    %1053 = arith.ori %1052, %1046 : i1
    %1054 = acir.extract %value_97 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc99)
    %1055 = arith.andi %ready_98, %1005 : i1 loc(#loc99)
    %1056 = arith.cmpi eq, %1054, %967 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc99)
    %1057 = arith.xori %1048, %true : i1
    %1058 = arith.ori %1057, %1051 : i1
    %1059 = arith.ori %1048, %1045 : i1
    %1060 = arith.xori %1045, %true : i1
    %1061 = arith.ori %1060, %1053 : i1
    %1062 = arith.andi %1058, %1061 : i1
    %1063 = arith.andi %1048, %1051 : i1
    %1064 = arith.andi %1053, %1055 : i1
    %1065 = arith.andi %1045, %1064 : i1
    %1066 = arith.ori %1063, %1065 : i1
    %1067 = arith.select %1048, %false_96, %1056 : i1
    %1068 = arith.xori %997, %true : i1
    %1069 = arith.ori %1068, %1040 : i1
    %1070 = arith.xori %997, %true : i1
    %1071 = arith.andi %1069, %1066 : i1
    %1072 = arith.ori %1070, %1071 : i1
    %1073 = arith.andi %1044, %1072 : i1
    %1074 = arith.andi %997, %1067 : i1
    %1075 = arith.andi %1069, %1066 : i1
    %1076 = arith.xori %1067, %true : i1
    %1077 = arith.andi %997, %1076 : i1
    %1078 = arith.andi %1069, %1066 : i1
    %1079 = arith.xori %1074, %true : i1
    %1080 = arith.ori %1079, %1075 : i1
    %1081 = acir.update %value_97, %true_95 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
    %1082 = arith.andi %ready_98, %1007 : i1 loc(#loc101)
    %1083 = acir.update %1081, %968 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc101)
    %1084 = arith.xori %1074, %true : i1 loc(#loc102)
    %1085 = arith.andi %1080, %1082 : i1 loc(#loc102)
    %1086 = arith.ori %1084, %1085 : i1 loc(#loc102)
    %1087 = arith.andi %1073, %1086 : i1 loc(#loc102)
    acir.path %1074 {branch = 20 : i64, kind = "branch", txn = 5 : i64} loc(#loc102)
    %1088 = arith.xori %1077, %true : i1
    %1089 = arith.ori %1088, %1078 : i1
    %1090 = arith.ori %1077, %1074 : i1
    %1091 = arith.xori %1074, %true : i1
    %1092 = arith.ori %1091, %1080 : i1
    %1093 = arith.andi %1089, %1092 : i1
    %1094 = arith.andi %997, %1069 : i1
    %1095 = arith.xori %994, %true : i1
    %1096 = arith.ori %1095, %995 : i1
    %1097 = arith.andi %965, %965 : i1 loc(#loc103)
    acir.path %994 {branch = 21 : i64, kind = "branch", txn = 5 : i64} loc(#loc103)
    acir.path %994 {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc103)
    %1098 = arith.xori %994, %true : i1 loc(#loc103)
    %1099 = arith.ori %1098, %1096 : i1 loc(#loc103)
    %1100 = arith.andi %1096, %965 : i1 loc(#loc103)
    %1101 = arith.andi %994, %1100 : i1 loc(#loc103)
    %1102 = arith.andi %1096, %965 : i1 loc(#loc103)
    %1103 = arith.andi %994, %1102 : i1 loc(#loc103)
    %true_99 = arith.constant {acir.source_type = "bool"} true
    %false_100 = arith.constant {acir.source_type = "bool"} false
    %value_101, %ready_102 = acir.read(%994) {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc95)
    %1104 = arith.xori %994, %true : i1 loc(#loc95)
    %1105 = arith.andi %1099, %ready_102 : i1 loc(#loc95)
    %1106 = arith.ori %1104, %1105 : i1 loc(#loc95)
    %1107 = arith.andi %1087, %1106 : i1 loc(#loc95)
    %1108 = acir.extract %value_101 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc96)
    %1109 = arith.xori %994, %true : i1
    %1110 = arith.andi %1099, %ready_102 : i1
    %1111 = arith.ori %1109, %1110 : i1
    %1112 = arith.andi %1107, %1111 : i1
    %1113 = arith.andi %994, %1108 : i1
    %1114 = arith.andi %1099, %ready_102 : i1
    %1115 = arith.xori %1108, %true : i1
    %1116 = arith.andi %994, %1115 : i1
    %1117 = arith.andi %1099, %ready_102 : i1
    %1118 = arith.xori %1116, %true : i1
    %1119 = arith.ori %1118, %1117 : i1
    %1120 = arith.xori %1113, %true : i1
    %1121 = arith.ori %1120, %1114 : i1
    %1122 = acir.extract %value_101 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc97)
    %1123 = acir.unary %1122 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc98)
    %1124 = arith.xori %1116, %true : i1
    %1125 = arith.ori %1124, %1119 : i1
    %1126 = arith.ori %1116, %1113 : i1
    %1127 = arith.xori %1113, %true : i1
    %1128 = arith.ori %1127, %1121 : i1
    %1129 = arith.andi %1125, %1128 : i1
    %1130 = arith.andi %1116, %1119 : i1
    %1131 = arith.andi %1121, %ready_102 : i1
    %1132 = arith.andi %1113, %1131 : i1
    %1133 = arith.ori %1130, %1132 : i1
    %1134 = arith.select %1116, %false_100, %1123 : i1
    %1135 = arith.xori %994, %true : i1
    %1136 = arith.ori %1135, %1099 : i1
    %1137 = arith.xori %994, %true : i1
    %1138 = arith.andi %1136, %1133 : i1
    %1139 = arith.ori %1137, %1138 : i1
    %1140 = arith.andi %1112, %1139 : i1
    %1141 = arith.andi %994, %1134 : i1
    %1142 = arith.andi %1136, %1133 : i1
    %1143 = arith.xori %1134, %true : i1
    %1144 = arith.andi %994, %1143 : i1
    %1145 = arith.andi %1136, %1133 : i1
    %1146 = arith.xori %1144, %true : i1
    %1147 = arith.ori %1146, %1145 : i1
    %1148 = arith.xori %1141, %true : i1
    %1149 = arith.ori %1148, %1142 : i1
    %1150 = acir.extract %value_101 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc99)
    %1151 = arith.andi %ready_102, %1101 : i1 loc(#loc99)
    %1152 = arith.cmpi eq, %1150, %967 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc99)
    %1153 = arith.xori %1144, %true : i1
    %1154 = arith.ori %1153, %1147 : i1
    %1155 = arith.ori %1144, %1141 : i1
    %1156 = arith.xori %1141, %true : i1
    %1157 = arith.ori %1156, %1149 : i1
    %1158 = arith.andi %1154, %1157 : i1
    %1159 = arith.andi %1144, %1147 : i1
    %1160 = arith.andi %1149, %1151 : i1
    %1161 = arith.andi %1141, %1160 : i1
    %1162 = arith.ori %1159, %1161 : i1
    %1163 = arith.select %1144, %false_100, %1152 : i1
    %1164 = arith.xori %994, %true : i1
    %1165 = arith.ori %1164, %1136 : i1
    %1166 = arith.xori %994, %true : i1
    %1167 = arith.andi %1165, %1162 : i1
    %1168 = arith.ori %1166, %1167 : i1
    %1169 = arith.andi %1140, %1168 : i1
    %1170 = arith.andi %994, %1163 : i1
    %1171 = arith.andi %1165, %1162 : i1
    %1172 = arith.xori %1163, %true : i1
    %1173 = arith.andi %994, %1172 : i1
    %1174 = arith.andi %1165, %1162 : i1
    %1175 = arith.xori %1170, %true : i1
    %1176 = arith.ori %1175, %1171 : i1
    %1177 = acir.update %value_101, %true_99 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
    %1178 = arith.andi %ready_102, %1103 : i1 loc(#loc101)
    %1179 = acir.update %1177, %968 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc101)
    %1180 = arith.xori %1170, %true : i1 loc(#loc102)
    %1181 = arith.andi %1176, %1178 : i1 loc(#loc102)
    %1182 = arith.ori %1180, %1181 : i1 loc(#loc102)
    %1183 = arith.andi %1169, %1182 : i1 loc(#loc102)
    acir.path %1170 {branch = 22 : i64, kind = "branch", txn = 5 : i64} loc(#loc102)
    %1184 = arith.xori %1173, %true : i1
    %1185 = arith.ori %1184, %1174 : i1
    %1186 = arith.ori %1173, %1170 : i1
    %1187 = arith.xori %1170, %true : i1
    %1188 = arith.ori %1187, %1176 : i1
    %1189 = arith.andi %1185, %1188 : i1
    %1190 = arith.andi %994, %1165 : i1
    %1191 = arith.xori %997, %true : i1
    %1192 = arith.ori %1191, %1000 : i1
    %1193 = arith.ori %997, %994 : i1
    %1194 = arith.xori %994, %true : i1
    %1195 = arith.ori %1194, %1096 : i1
    %1196 = arith.andi %1192, %1195 : i1
    %1197 = arith.xori %982, %true : i1
    %1198 = arith.ori %1197, %983 : i1
    %1199 = arith.andi %965, %965 : i1 loc(#loc104)
    acir.path %982 {branch = 23 : i64, kind = "branch", txn = 5 : i64} loc(#loc104)
    acir.path %982 {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc104)
    %1200 = arith.xori %982, %true : i1 loc(#loc104)
    %1201 = arith.ori %1200, %1198 : i1 loc(#loc104)
    %1202 = arith.andi %1198, %965 : i1 loc(#loc104)
    %1203 = arith.andi %982, %1202 : i1 loc(#loc104)
    %1204 = arith.andi %1198, %965 : i1 loc(#loc104)
    %1205 = arith.andi %982, %1204 : i1 loc(#loc104)
    %true_103 = arith.constant {acir.source_type = "bool"} true
    %false_104 = arith.constant {acir.source_type = "bool"} false
    %value_105, %ready_106 = acir.read(%982) {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc95)
    %1206 = arith.xori %982, %true : i1 loc(#loc95)
    %1207 = arith.andi %1201, %ready_106 : i1 loc(#loc95)
    %1208 = arith.ori %1206, %1207 : i1 loc(#loc95)
    %1209 = arith.andi %1183, %1208 : i1 loc(#loc95)
    %1210 = acir.extract %value_105 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc96)
    %1211 = arith.xori %982, %true : i1
    %1212 = arith.andi %1201, %ready_106 : i1
    %1213 = arith.ori %1211, %1212 : i1
    %1214 = arith.andi %1209, %1213 : i1
    %1215 = arith.andi %982, %1210 : i1
    %1216 = arith.andi %1201, %ready_106 : i1
    %1217 = arith.xori %1210, %true : i1
    %1218 = arith.andi %982, %1217 : i1
    %1219 = arith.andi %1201, %ready_106 : i1
    %1220 = arith.xori %1218, %true : i1
    %1221 = arith.ori %1220, %1219 : i1
    %1222 = arith.xori %1215, %true : i1
    %1223 = arith.ori %1222, %1216 : i1
    %1224 = acir.extract %value_105 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc97)
    %1225 = acir.unary %1224 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc98)
    %1226 = arith.xori %1218, %true : i1
    %1227 = arith.ori %1226, %1221 : i1
    %1228 = arith.ori %1218, %1215 : i1
    %1229 = arith.xori %1215, %true : i1
    %1230 = arith.ori %1229, %1223 : i1
    %1231 = arith.andi %1227, %1230 : i1
    %1232 = arith.andi %1218, %1221 : i1
    %1233 = arith.andi %1223, %ready_106 : i1
    %1234 = arith.andi %1215, %1233 : i1
    %1235 = arith.ori %1232, %1234 : i1
    %1236 = arith.select %1218, %false_104, %1225 : i1
    %1237 = arith.xori %982, %true : i1
    %1238 = arith.ori %1237, %1201 : i1
    %1239 = arith.xori %982, %true : i1
    %1240 = arith.andi %1238, %1235 : i1
    %1241 = arith.ori %1239, %1240 : i1
    %1242 = arith.andi %1214, %1241 : i1
    %1243 = arith.andi %982, %1236 : i1
    %1244 = arith.andi %1238, %1235 : i1
    %1245 = arith.xori %1236, %true : i1
    %1246 = arith.andi %982, %1245 : i1
    %1247 = arith.andi %1238, %1235 : i1
    %1248 = arith.xori %1246, %true : i1
    %1249 = arith.ori %1248, %1247 : i1
    %1250 = arith.xori %1243, %true : i1
    %1251 = arith.ori %1250, %1244 : i1
    %1252 = acir.extract %value_105 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc99)
    %1253 = arith.andi %ready_106, %1203 : i1 loc(#loc99)
    %1254 = arith.cmpi eq, %1252, %967 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc99)
    %1255 = arith.xori %1246, %true : i1
    %1256 = arith.ori %1255, %1249 : i1
    %1257 = arith.ori %1246, %1243 : i1
    %1258 = arith.xori %1243, %true : i1
    %1259 = arith.ori %1258, %1251 : i1
    %1260 = arith.andi %1256, %1259 : i1
    %1261 = arith.andi %1246, %1249 : i1
    %1262 = arith.andi %1251, %1253 : i1
    %1263 = arith.andi %1243, %1262 : i1
    %1264 = arith.ori %1261, %1263 : i1
    %1265 = arith.select %1246, %false_104, %1254 : i1
    %1266 = arith.xori %982, %true : i1
    %1267 = arith.ori %1266, %1238 : i1
    %1268 = arith.xori %982, %true : i1
    %1269 = arith.andi %1267, %1264 : i1
    %1270 = arith.ori %1268, %1269 : i1
    %1271 = arith.andi %1242, %1270 : i1
    %1272 = arith.andi %982, %1265 : i1
    %1273 = arith.andi %1267, %1264 : i1
    %1274 = arith.xori %1265, %true : i1
    %1275 = arith.andi %982, %1274 : i1
    %1276 = arith.andi %1267, %1264 : i1
    %1277 = arith.xori %1272, %true : i1
    %1278 = arith.ori %1277, %1273 : i1
    %1279 = acir.update %value_105, %true_103 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
    %1280 = arith.andi %ready_106, %1205 : i1 loc(#loc101)
    %1281 = acir.update %1279, %968 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc101)
    %1282 = arith.xori %1272, %true : i1 loc(#loc102)
    %1283 = arith.andi %1278, %1280 : i1 loc(#loc102)
    %1284 = arith.ori %1282, %1283 : i1 loc(#loc102)
    %1285 = arith.andi %1271, %1284 : i1 loc(#loc102)
    acir.path %1272 {branch = 24 : i64, kind = "branch", txn = 5 : i64} loc(#loc102)
    %1286 = arith.xori %1275, %true : i1
    %1287 = arith.ori %1286, %1276 : i1
    %1288 = arith.ori %1275, %1272 : i1
    %1289 = arith.xori %1272, %true : i1
    %1290 = arith.ori %1289, %1278 : i1
    %1291 = arith.andi %1287, %1290 : i1
    %1292 = arith.andi %982, %1267 : i1
    %1293 = arith.xori %985, %true : i1
    %1294 = arith.ori %1293, %988 : i1
    %1295 = arith.ori %985, %982 : i1
    %1296 = arith.xori %982, %true : i1
    %1297 = arith.ori %1296, %1198 : i1
    %1298 = arith.andi %1294, %1297 : i1
    %1299 = arith.xori %972, %true : i1
    %1300 = arith.ori %1299, %965 : i1
    %1301 = arith.andi %965, %965 : i1 loc(#loc105)
    acir.path %972 {branch = 25 : i64, kind = "branch", txn = 5 : i64} loc(#loc105)
    acir.path %972 {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc105)
    %1302 = arith.xori %972, %true : i1 loc(#loc105)
    %1303 = arith.ori %1302, %1300 : i1 loc(#loc105)
    %1304 = arith.andi %1300, %965 : i1 loc(#loc105)
    %1305 = arith.andi %972, %1304 : i1 loc(#loc105)
    %1306 = arith.andi %1300, %965 : i1 loc(#loc105)
    %1307 = arith.andi %972, %1306 : i1 loc(#loc105)
    %true_107 = arith.constant {acir.source_type = "bool"} true
    %false_108 = arith.constant {acir.source_type = "bool"} false
    %value_109, %ready_110 = acir.read(%972) {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc95)
    %1308 = arith.xori %972, %true : i1 loc(#loc95)
    %1309 = arith.andi %1303, %ready_110 : i1 loc(#loc95)
    %1310 = arith.ori %1308, %1309 : i1 loc(#loc95)
    %1311 = arith.andi %1285, %1310 : i1 loc(#loc95)
    %1312 = acir.extract %value_109 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc96)
    %1313 = arith.xori %972, %true : i1
    %1314 = arith.andi %1303, %ready_110 : i1
    %1315 = arith.ori %1313, %1314 : i1
    %1316 = arith.andi %1311, %1315 : i1
    %1317 = arith.andi %972, %1312 : i1
    %1318 = arith.andi %1303, %ready_110 : i1
    %1319 = arith.xori %1312, %true : i1
    %1320 = arith.andi %972, %1319 : i1
    %1321 = arith.andi %1303, %ready_110 : i1
    %1322 = arith.xori %1320, %true : i1
    %1323 = arith.ori %1322, %1321 : i1
    %1324 = arith.xori %1317, %true : i1
    %1325 = arith.ori %1324, %1318 : i1
    %1326 = acir.extract %value_109 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc97)
    %1327 = acir.unary %1326 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc98)
    %1328 = arith.xori %1320, %true : i1
    %1329 = arith.ori %1328, %1323 : i1
    %1330 = arith.ori %1320, %1317 : i1
    %1331 = arith.xori %1317, %true : i1
    %1332 = arith.ori %1331, %1325 : i1
    %1333 = arith.andi %1329, %1332 : i1
    %1334 = arith.andi %1320, %1323 : i1
    %1335 = arith.andi %1325, %ready_110 : i1
    %1336 = arith.andi %1317, %1335 : i1
    %1337 = arith.ori %1334, %1336 : i1
    %1338 = arith.select %1320, %false_108, %1327 : i1
    %1339 = arith.xori %972, %true : i1
    %1340 = arith.ori %1339, %1303 : i1
    %1341 = arith.xori %972, %true : i1
    %1342 = arith.andi %1340, %1337 : i1
    %1343 = arith.ori %1341, %1342 : i1
    %1344 = arith.andi %1316, %1343 : i1
    %1345 = arith.andi %972, %1338 : i1
    %1346 = arith.andi %1340, %1337 : i1
    %1347 = arith.xori %1338, %true : i1
    %1348 = arith.andi %972, %1347 : i1
    %1349 = arith.andi %1340, %1337 : i1
    %1350 = arith.xori %1348, %true : i1
    %1351 = arith.ori %1350, %1349 : i1
    %1352 = arith.xori %1345, %true : i1
    %1353 = arith.ori %1352, %1346 : i1
    %1354 = acir.extract %value_109 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc99)
    %1355 = arith.andi %ready_110, %1305 : i1 loc(#loc99)
    %1356 = arith.cmpi eq, %1354, %967 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc99)
    %1357 = arith.xori %1348, %true : i1
    %1358 = arith.ori %1357, %1351 : i1
    %1359 = arith.ori %1348, %1345 : i1
    %1360 = arith.xori %1345, %true : i1
    %1361 = arith.ori %1360, %1353 : i1
    %1362 = arith.andi %1358, %1361 : i1
    %1363 = arith.andi %1348, %1351 : i1
    %1364 = arith.andi %1353, %1355 : i1
    %1365 = arith.andi %1345, %1364 : i1
    %1366 = arith.ori %1363, %1365 : i1
    %1367 = arith.select %1348, %false_108, %1356 : i1
    %1368 = arith.xori %972, %true : i1
    %1369 = arith.ori %1368, %1340 : i1
    %1370 = arith.xori %972, %true : i1
    %1371 = arith.andi %1369, %1366 : i1
    %1372 = arith.ori %1370, %1371 : i1
    %1373 = arith.andi %1344, %1372 : i1
    %1374 = arith.andi %972, %1367 : i1
    %1375 = arith.andi %1369, %1366 : i1
    %1376 = arith.xori %1367, %true : i1
    %1377 = arith.andi %972, %1376 : i1
    %1378 = arith.andi %1369, %1366 : i1
    %1379 = arith.xori %1374, %true : i1
    %1380 = arith.ori %1379, %1375 : i1
    %1381 = acir.update %value_109, %true_107 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
    %1382 = arith.andi %ready_110, %1307 : i1 loc(#loc101)
    %1383 = acir.update %1381, %968 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc101)
    %1384 = arith.xori %1374, %true : i1 loc(#loc102)
    %1385 = arith.andi %1380, %1382 : i1 loc(#loc102)
    %1386 = arith.ori %1384, %1385 : i1 loc(#loc102)
    %1387 = arith.andi %1373, %1386 : i1 loc(#loc102)
    acir.path %1374 {branch = 26 : i64, kind = "branch", txn = 5 : i64} loc(#loc102)
    %1388 = arith.xori %1377, %true : i1
    %1389 = arith.ori %1388, %1378 : i1
    %1390 = arith.ori %1377, %1374 : i1
    %1391 = arith.xori %1374, %true : i1
    %1392 = arith.ori %1391, %1380 : i1
    %1393 = arith.andi %1389, %1392 : i1
    %1394 = arith.andi %972, %1369 : i1
    %1395 = arith.xori %974, %true : i1
    %1396 = arith.ori %1395, %976 : i1
    %1397 = arith.ori %974, %972 : i1
    %1398 = arith.xori %972, %true : i1
    %1399 = arith.ori %1398, %1300 : i1
    %1400 = arith.andi %1396, %1399 : i1
    %1401 = acir.extract %966 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc106)
    %1402 = acir.extract %966 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc107)
    %1403 = arith.andi %965, %965 : i1 loc(#loc108)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc108)
    %true_111 = arith.constant {acir.source_type = "bool"} true
    %false_112 = arith.constant {acir.source_type = "bool"} false
    %value_113, %ready_114 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc109)
    %1404 = arith.andi %1387, %ready_114 : i1 loc(#loc109)
    %1405 = acir.extract %value_113 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc110)
    %1406 = arith.andi %1404, %ready_114 : i1
    %1407 = arith.xori %1405, %true : i1
    %1408 = arith.xori %1407, %true : i1
    %1409 = arith.ori %1408, %ready_114 : i1
    %1410 = arith.xori %1405, %true : i1
    %1411 = arith.ori %1410, %ready_114 : i1
    %1412 = acir.extract %value_113 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc111)
    %1413 = acir.extract %1412 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc111)
    %1414 = acir.unary %1413 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc112)
    %1415 = arith.xori %1405, %true : i1
    %1416 = arith.andi %1411, %ready_114 : i1
    %1417 = arith.ori %1415, %1416 : i1
    %1418 = arith.andi %1406, %1417 : i1
    %1419 = arith.andi %1405, %1414 : i1
    %1420 = arith.andi %1411, %ready_114 : i1
    %1421 = arith.xori %1414, %true : i1
    %1422 = arith.andi %1405, %1421 : i1
    %1423 = arith.andi %1411, %ready_114 : i1
    %1424 = arith.xori %1422, %true : i1
    %1425 = arith.ori %1424, %1423 : i1
    %1426 = arith.xori %1419, %true : i1
    %1427 = arith.ori %1426, %1420 : i1
    %1428 = acir.extract %value_113 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc113)
    %1429 = acir.extract %1428 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc113)
    %1430 = arith.andi %ready_114, %965 : i1 loc(#loc113)
    %1431 = arith.cmpi eq, %1429, %1401 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc113)
    %1432 = arith.xori %1422, %true : i1
    %1433 = arith.ori %1432, %1425 : i1
    %1434 = arith.ori %1422, %1419 : i1
    %1435 = arith.xori %1419, %true : i1
    %1436 = arith.ori %1435, %1427 : i1
    %1437 = arith.andi %1433, %1436 : i1
    %1438 = arith.andi %1422, %1425 : i1
    %1439 = arith.andi %1427, %1430 : i1
    %1440 = arith.andi %1419, %1439 : i1
    %1441 = arith.ori %1438, %1440 : i1
    %1442 = arith.select %1422, %false_112, %1431 : i1
    %1443 = arith.xori %1405, %true : i1
    %1444 = arith.ori %1443, %1411 : i1
    %1445 = arith.xori %1405, %true : i1
    %1446 = arith.andi %1444, %1441 : i1
    %1447 = arith.ori %1445, %1446 : i1
    %1448 = arith.andi %1418, %1447 : i1
    %1449 = arith.andi %1405, %1442 : i1
    %1450 = arith.andi %1444, %1441 : i1
    %1451 = arith.xori %1442, %true : i1
    %1452 = arith.andi %1405, %1451 : i1
    %1453 = arith.andi %1444, %1441 : i1
    %1454 = arith.xori %1452, %true : i1
    %1455 = arith.ori %1454, %1453 : i1
    %1456 = arith.xori %1449, %true : i1
    %1457 = arith.ori %1456, %1450 : i1
    %1458 = arith.andi %965, %965 : i1 loc(#loc114)
    %1459 = acir.aggregate(%true_111, %1401, %1402) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc114)
    %1460 = arith.andi %ready_114, %1458 : i1 loc(#loc115)
    %1461 = acir.update %value_113, %1459 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc115)
    %1462 = arith.xori %1452, %true : i1
    %1463 = arith.ori %1462, %1455 : i1
    %1464 = arith.ori %1452, %1449 : i1
    %1465 = arith.xori %1449, %true : i1
    %1466 = arith.ori %1465, %1457 : i1
    %1467 = arith.andi %1463, %1466 : i1
    %1468 = arith.andi %1455, %ready_114 : i1
    %1469 = arith.andi %1452, %1468 : i1
    %1470 = arith.andi %1457, %1460 : i1
    %1471 = arith.andi %1449, %1470 : i1
    %1472 = arith.ori %1469, %1471 : i1
    %1473 = arith.select %1452, %value_113, %1461 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1474 = arith.xori %1405, %true : i1
    %1475 = arith.ori %1474, %1444 : i1
    %1476 = acir.extract %1473 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc116)
    %1477 = acir.extract %1476 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc116)
    %1478 = acir.unary %1477 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc117)
    %1479 = arith.xori %1405, %true : i1
    %1480 = arith.andi %1475, %1472 : i1
    %1481 = arith.ori %1479, %1480 : i1
    %1482 = arith.andi %1448, %1481 : i1
    %1483 = arith.andi %1405, %1478 : i1
    %1484 = arith.andi %1475, %1472 : i1
    %1485 = arith.xori %1478, %true : i1
    %1486 = arith.andi %1405, %1485 : i1
    %1487 = arith.andi %1475, %1472 : i1
    %1488 = arith.xori %1486, %true : i1
    %1489 = arith.ori %1488, %1487 : i1
    %1490 = arith.xori %1483, %true : i1
    %1491 = arith.ori %1490, %1484 : i1
    %1492 = acir.extract %1473 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc118)
    %1493 = acir.extract %1492 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc118)
    %1494 = arith.andi %1472, %965 : i1 loc(#loc118)
    %1495 = arith.cmpi eq, %1493, %1401 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc118)
    %1496 = arith.xori %1486, %true : i1
    %1497 = arith.ori %1496, %1489 : i1
    %1498 = arith.ori %1486, %1483 : i1
    %1499 = arith.xori %1483, %true : i1
    %1500 = arith.ori %1499, %1491 : i1
    %1501 = arith.andi %1497, %1500 : i1
    %1502 = arith.andi %1486, %1489 : i1
    %1503 = arith.andi %1491, %1494 : i1
    %1504 = arith.andi %1483, %1503 : i1
    %1505 = arith.ori %1502, %1504 : i1
    %1506 = arith.select %1486, %false_112, %1495 : i1
    %1507 = arith.xori %1405, %true : i1
    %1508 = arith.ori %1507, %1475 : i1
    %1509 = arith.andi %1505, %1441 : i1
    %1510 = arith.select %1506, %true_111, %1442 : i1
    %1511 = arith.xori %1405, %true : i1
    %1512 = arith.andi %1508, %1505 : i1
    %1513 = arith.ori %1511, %1512 : i1
    %1514 = arith.andi %1482, %1513 : i1
    %1515 = arith.andi %1405, %1506 : i1
    %1516 = arith.andi %1508, %1505 : i1
    %1517 = arith.xori %1506, %true : i1
    %1518 = arith.andi %1405, %1517 : i1
    %1519 = arith.andi %1508, %1505 : i1
    %1520 = arith.xori %1518, %true : i1
    %1521 = arith.ori %1520, %1519 : i1
    %1522 = arith.xori %1515, %true : i1
    %1523 = arith.ori %1522, %1516 : i1
    %1524 = arith.andi %965, %965 : i1 loc(#loc119)
    %1525 = acir.aggregate(%true_111, %1401, %1402) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc119)
    %1526 = arith.andi %1472, %1524 : i1 loc(#loc120)
    %1527 = acir.update %1473, %1525 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc120)
    %1528 = arith.xori %1518, %true : i1
    %1529 = arith.ori %1528, %1521 : i1
    %1530 = arith.ori %1518, %1515 : i1
    %1531 = arith.xori %1515, %true : i1
    %1532 = arith.ori %1531, %1523 : i1
    %1533 = arith.andi %1529, %1532 : i1
    %1534 = arith.andi %1521, %1472 : i1
    %1535 = arith.andi %1518, %1534 : i1
    %1536 = arith.andi %1523, %1526 : i1
    %1537 = arith.andi %1515, %1536 : i1
    %1538 = arith.ori %1535, %1537 : i1
    %1539 = arith.select %1518, %1473, %1527 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1540 = arith.xori %1405, %true : i1
    %1541 = arith.ori %1540, %1508 : i1
    %1542 = arith.andi %1509, %1538 : i1
    %1543 = arith.xori %1407, %true : i1
    %1544 = arith.ori %1543, %1409 : i1
    %1545 = arith.ori %1407, %1405 : i1
    %1546 = arith.xori %1405, %true : i1
    %1547 = arith.ori %1546, %1541 : i1
    %1548 = arith.andi %1544, %1547 : i1
    %1549 = arith.andi %1407, %1409 : i1
    %1550 = arith.andi %1541, %1509 : i1
    %1551 = arith.andi %1405, %1550 : i1
    %1552 = arith.ori %1549, %1551 : i1
    %1553 = arith.select %1407, %false_112, %1510 : i1
    %1554 = arith.andi %1409, %ready_114 : i1
    %1555 = arith.andi %1407, %1554 : i1
    %1556 = arith.andi %1541, %1538 : i1
    %1557 = arith.andi %1405, %1556 : i1
    %1558 = arith.ori %1555, %1557 : i1
    %1559 = arith.select %1407, %value_113, %1539 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1560 = arith.andi %1514, %1552 : i1
    %1561 = arith.xori %1553, %true : i1
    %1562 = arith.xori %1553, %true : i1
    %1563 = arith.ori %1562, %1552 : i1
    %1564 = arith.xori %1553, %true : i1 loc(#loc121)
    %1565 = arith.andi %1563, %1558 : i1 loc(#loc121)
    %1566 = arith.ori %1564, %1565 : i1 loc(#loc121)
    %1567 = arith.andi %1560, %1566 : i1 loc(#loc121)
    acir.path %1553 {branch = 27 : i64, kind = "branch", txn = 5 : i64} loc(#loc121)
    %1568 = arith.xori %1561, %true : i1
    %1569 = arith.ori %1568, %1552 : i1
    %1570 = arith.ori %1561, %1553 : i1
    %1571 = arith.xori %1553, %true : i1
    %1572 = arith.ori %1571, %1563 : i1
    %1573 = arith.andi %1569, %1572 : i1
    %1574 = acir.extract %966 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc122)
    %1575 = acir.extract %966 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc123)
    %1576 = arith.andi %965, %965 : i1 loc(#loc124)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc124)
    %true_115 = arith.constant {acir.source_type = "bool"} true
    %false_116 = arith.constant {acir.source_type = "bool"} false
    %value_117, %ready_118 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc109)
    %1577 = arith.andi %1567, %ready_118 : i1 loc(#loc109)
    %1578 = acir.extract %value_117 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc110)
    %1579 = arith.andi %1577, %ready_118 : i1
    %1580 = arith.xori %1578, %true : i1
    %1581 = arith.xori %1580, %true : i1
    %1582 = arith.ori %1581, %ready_118 : i1
    %1583 = arith.xori %1578, %true : i1
    %1584 = arith.ori %1583, %ready_118 : i1
    %1585 = acir.extract %value_117 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc111)
    %1586 = acir.extract %1585 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc111)
    %1587 = acir.unary %1586 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc112)
    %1588 = arith.xori %1578, %true : i1
    %1589 = arith.andi %1584, %ready_118 : i1
    %1590 = arith.ori %1588, %1589 : i1
    %1591 = arith.andi %1579, %1590 : i1
    %1592 = arith.andi %1578, %1587 : i1
    %1593 = arith.andi %1584, %ready_118 : i1
    %1594 = arith.xori %1587, %true : i1
    %1595 = arith.andi %1578, %1594 : i1
    %1596 = arith.andi %1584, %ready_118 : i1
    %1597 = arith.xori %1595, %true : i1
    %1598 = arith.ori %1597, %1596 : i1
    %1599 = arith.xori %1592, %true : i1
    %1600 = arith.ori %1599, %1593 : i1
    %1601 = acir.extract %value_117 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc113)
    %1602 = acir.extract %1601 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc113)
    %1603 = arith.andi %ready_118, %965 : i1 loc(#loc113)
    %1604 = arith.cmpi eq, %1602, %1574 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc113)
    %1605 = arith.xori %1595, %true : i1
    %1606 = arith.ori %1605, %1598 : i1
    %1607 = arith.ori %1595, %1592 : i1
    %1608 = arith.xori %1592, %true : i1
    %1609 = arith.ori %1608, %1600 : i1
    %1610 = arith.andi %1606, %1609 : i1
    %1611 = arith.andi %1595, %1598 : i1
    %1612 = arith.andi %1600, %1603 : i1
    %1613 = arith.andi %1592, %1612 : i1
    %1614 = arith.ori %1611, %1613 : i1
    %1615 = arith.select %1595, %false_116, %1604 : i1
    %1616 = arith.xori %1578, %true : i1
    %1617 = arith.ori %1616, %1584 : i1
    %1618 = arith.xori %1578, %true : i1
    %1619 = arith.andi %1617, %1614 : i1
    %1620 = arith.ori %1618, %1619 : i1
    %1621 = arith.andi %1591, %1620 : i1
    %1622 = arith.andi %1578, %1615 : i1
    %1623 = arith.andi %1617, %1614 : i1
    %1624 = arith.xori %1615, %true : i1
    %1625 = arith.andi %1578, %1624 : i1
    %1626 = arith.andi %1617, %1614 : i1
    %1627 = arith.xori %1625, %true : i1
    %1628 = arith.ori %1627, %1626 : i1
    %1629 = arith.xori %1622, %true : i1
    %1630 = arith.ori %1629, %1623 : i1
    %1631 = arith.andi %965, %965 : i1 loc(#loc114)
    %1632 = acir.aggregate(%true_115, %1574, %1575) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc114)
    %1633 = arith.andi %ready_118, %1631 : i1 loc(#loc115)
    %1634 = acir.update %value_117, %1632 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc115)
    %1635 = arith.xori %1625, %true : i1
    %1636 = arith.ori %1635, %1628 : i1
    %1637 = arith.ori %1625, %1622 : i1
    %1638 = arith.xori %1622, %true : i1
    %1639 = arith.ori %1638, %1630 : i1
    %1640 = arith.andi %1636, %1639 : i1
    %1641 = arith.andi %1628, %ready_118 : i1
    %1642 = arith.andi %1625, %1641 : i1
    %1643 = arith.andi %1630, %1633 : i1
    %1644 = arith.andi %1622, %1643 : i1
    %1645 = arith.ori %1642, %1644 : i1
    %1646 = arith.select %1625, %value_117, %1634 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1647 = arith.xori %1578, %true : i1
    %1648 = arith.ori %1647, %1617 : i1
    %1649 = acir.extract %1646 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc116)
    %1650 = acir.extract %1649 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc116)
    %1651 = acir.unary %1650 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc117)
    %1652 = arith.xori %1578, %true : i1
    %1653 = arith.andi %1648, %1645 : i1
    %1654 = arith.ori %1652, %1653 : i1
    %1655 = arith.andi %1621, %1654 : i1
    %1656 = arith.andi %1578, %1651 : i1
    %1657 = arith.andi %1648, %1645 : i1
    %1658 = arith.xori %1651, %true : i1
    %1659 = arith.andi %1578, %1658 : i1
    %1660 = arith.andi %1648, %1645 : i1
    %1661 = arith.xori %1659, %true : i1
    %1662 = arith.ori %1661, %1660 : i1
    %1663 = arith.xori %1656, %true : i1
    %1664 = arith.ori %1663, %1657 : i1
    %1665 = acir.extract %1646 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc118)
    %1666 = acir.extract %1665 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc118)
    %1667 = arith.andi %1645, %965 : i1 loc(#loc118)
    %1668 = arith.cmpi eq, %1666, %1574 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc118)
    %1669 = arith.xori %1659, %true : i1
    %1670 = arith.ori %1669, %1662 : i1
    %1671 = arith.ori %1659, %1656 : i1
    %1672 = arith.xori %1656, %true : i1
    %1673 = arith.ori %1672, %1664 : i1
    %1674 = arith.andi %1670, %1673 : i1
    %1675 = arith.andi %1659, %1662 : i1
    %1676 = arith.andi %1664, %1667 : i1
    %1677 = arith.andi %1656, %1676 : i1
    %1678 = arith.ori %1675, %1677 : i1
    %1679 = arith.select %1659, %false_116, %1668 : i1
    %1680 = arith.xori %1578, %true : i1
    %1681 = arith.ori %1680, %1648 : i1
    %1682 = arith.andi %1678, %1614 : i1
    %1683 = arith.select %1679, %true_115, %1615 : i1
    %1684 = arith.xori %1578, %true : i1
    %1685 = arith.andi %1681, %1678 : i1
    %1686 = arith.ori %1684, %1685 : i1
    %1687 = arith.andi %1655, %1686 : i1
    %1688 = arith.andi %1578, %1679 : i1
    %1689 = arith.andi %1681, %1678 : i1
    %1690 = arith.xori %1679, %true : i1
    %1691 = arith.andi %1578, %1690 : i1
    %1692 = arith.andi %1681, %1678 : i1
    %1693 = arith.xori %1691, %true : i1
    %1694 = arith.ori %1693, %1692 : i1
    %1695 = arith.xori %1688, %true : i1
    %1696 = arith.ori %1695, %1689 : i1
    %1697 = arith.andi %965, %965 : i1 loc(#loc119)
    %1698 = acir.aggregate(%true_115, %1574, %1575) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc119)
    %1699 = arith.andi %1645, %1697 : i1 loc(#loc120)
    %1700 = acir.update %1646, %1698 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc120)
    %1701 = arith.xori %1691, %true : i1
    %1702 = arith.ori %1701, %1694 : i1
    %1703 = arith.ori %1691, %1688 : i1
    %1704 = arith.xori %1688, %true : i1
    %1705 = arith.ori %1704, %1696 : i1
    %1706 = arith.andi %1702, %1705 : i1
    %1707 = arith.andi %1694, %1645 : i1
    %1708 = arith.andi %1691, %1707 : i1
    %1709 = arith.andi %1696, %1699 : i1
    %1710 = arith.andi %1688, %1709 : i1
    %1711 = arith.ori %1708, %1710 : i1
    %1712 = arith.select %1691, %1646, %1700 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1713 = arith.xori %1578, %true : i1
    %1714 = arith.ori %1713, %1681 : i1
    %1715 = arith.andi %1682, %1711 : i1
    %1716 = arith.xori %1580, %true : i1
    %1717 = arith.ori %1716, %1582 : i1
    %1718 = arith.ori %1580, %1578 : i1
    %1719 = arith.xori %1578, %true : i1
    %1720 = arith.ori %1719, %1714 : i1
    %1721 = arith.andi %1717, %1720 : i1
    %1722 = arith.andi %1580, %1582 : i1
    %1723 = arith.andi %1714, %1682 : i1
    %1724 = arith.andi %1578, %1723 : i1
    %1725 = arith.ori %1722, %1724 : i1
    %1726 = arith.select %1580, %false_116, %1683 : i1
    %1727 = arith.andi %1582, %ready_118 : i1
    %1728 = arith.andi %1580, %1727 : i1
    %1729 = arith.andi %1714, %1711 : i1
    %1730 = arith.andi %1578, %1729 : i1
    %1731 = arith.ori %1728, %1730 : i1
    %1732 = arith.select %1580, %value_117, %1712 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %1733 = arith.andi %1687, %1725 : i1
    %1734 = arith.xori %1726, %true : i1
    %1735 = arith.xori %1726, %true : i1
    %1736 = arith.ori %1735, %1725 : i1
    %1737 = arith.xori %1726, %true : i1 loc(#loc121)
    %1738 = arith.andi %1736, %1731 : i1 loc(#loc121)
    %1739 = arith.ori %1737, %1738 : i1 loc(#loc121)
    %1740 = arith.andi %1733, %1739 : i1 loc(#loc121)
    acir.path %1726 {branch = 28 : i64, kind = "branch", txn = 5 : i64} loc(#loc121)
    %1741 = arith.xori %1734, %true : i1
    %1742 = arith.ori %1741, %1725 : i1
    %1743 = arith.ori %1734, %1726 : i1
    %1744 = arith.xori %1726, %true : i1
    %1745 = arith.ori %1744, %1736 : i1
    %1746 = arith.andi %1742, %1745 : i1
    %1747 = arith.xori %965, %true : i1 loc(#loc125)
    acir.path %965 {branch = 29 : i64, kind = "branch", txn = 5 : i64} loc(#loc125)
    %1748 = acir.fire "top.cdb.broadcast#5" %1740 {successors = [2, 4, 8], txn = 5 : i64} loc(#loc125)
    %1749 = arith.andi %1748, %916 : i1 loc(#loc125)
    acir.effect %1749() {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.multiplier.out", txn = 5 : i64} : (i1) -> () loc(#loc83)
    %1750 = arith.andi %1748, %914 : i1 loc(#loc125)
    acir.effect %1750() {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.adder.out", txn = 5 : i64} : (i1) -> () loc(#loc84)
    %1751 = arith.andi %1748, %1074 : i1 loc(#loc125)
    acir.effect %1751(%1083) {branch = 20 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot3:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc102)
    %1752 = arith.andi %1748, %1170 : i1 loc(#loc125)
    acir.effect %1752(%1179) {branch = 22 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot2:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc102)
    %1753 = arith.andi %1748, %1272 : i1 loc(#loc125)
    acir.effect %1753(%1281) {branch = 24 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot1:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc102)
    %1754 = arith.andi %1748, %1374 : i1 loc(#loc125)
    acir.effect %1754(%1383) {branch = 26 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob.slot0:complete"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc102)
    %1755 = arith.andi %1748, %1553 : i1 loc(#loc125)
    acir.effect %1755(%1559) {branch = 27 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc121)
    %1756 = arith.andi %1748, %1726 : i1 loc(#loc125)
    acir.effect %1756(%1732) {branch = 28 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc121)
    %1757 = arith.andi %1748, %965 : i1 loc(#loc125)
    acir.effect %1757(%966) {branch = 29 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.cdb.event", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> () loc(#loc125)
    %false_119 = arith.constant {acir.source_type = "bool"} false
    %value_120, %ready_121 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.adder.out", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc126)
    %1758 = arith.xori %value_120, %true : i1
    %1759 = arith.xori %1758, %true : i1
    %1760 = arith.ori %1759, %ready_121 : i1
    %1761 = arith.xori %value_120, %true : i1
    %1762 = arith.ori %1761, %ready_121 : i1
    acir.path %value_120 {branch = 30 : i64, kind = "branch", txn = -1 : i64} loc(#loc127)
    %value_122, %ready_123 = acir.read(%value_120) {branch = 30 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.multiplier.out", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc127)
    %1763 = arith.xori %1758, %true : i1
    %1764 = arith.ori %1763, %1760 : i1
    %1765 = arith.ori %1758, %value_120 : i1
    %1766 = arith.xori %value_120, %true : i1
    %1767 = arith.ori %1766, %1762 : i1
    %1768 = arith.andi %1764, %1767 : i1
    %1769 = arith.andi %1758, %1760 : i1
    %1770 = arith.andi %1762, %ready_123 : i1
    %1771 = arith.andi %value_120, %1770 : i1
    %1772 = arith.ori %1769, %1771 : i1
    %1773 = arith.select %1758, %false_119, %value_122 : i1
    %1774 = arith.xori %1773, %true : i1
    %1775 = arith.xori %1773, %true : i1
    %1776 = arith.ori %1775, %1772 : i1
    %1777 = arith.andi %1773, %1776 : i1 loc(#loc128)
    acir.path %1773 {branch = 31 : i64, kind = "branch", txn = 6 : i64} loc(#loc128)
    acir.path %1773 {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc128)
    %1778 = arith.xori %1773, %true : i1 loc(#loc128)
    %1779 = arith.ori %1778, %1776 : i1 loc(#loc128)
    %1780 = arith.andi %1773, %1776 : i1 loc(#loc128)
    %true_124 = arith.constant {acir.source_type = "bool"} true loc(#loc129)
    %value_125, %ready_126 = acir.read(%1773) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "peek", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, i1) loc(#loc130)
    %1781 = arith.xori %1773, %true : i1 loc(#loc130)
    %1782 = arith.andi %1779, %ready_126 : i1 loc(#loc130)
    %1783 = arith.ori %1781, %1782 : i1 loc(#loc130)
    %1784 = arith.andi %1777, %1783 : i1 loc(#loc130)
    %1785 = arith.xori %1773, %true : i1 loc(#loc130)
    %1786 = arith.andi %1779, %ready_126 : i1 loc(#loc130)
    %1787 = arith.ori %1785, %1786 : i1 loc(#loc130)
    %1788 = arith.andi %1784, %1787 : i1 loc(#loc130)
    %value_127, %ready_128 = acir.read(%1773) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "empty", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (i1, i1) loc(#loc130)
    %1789 = arith.xori %1773, %true : i1 loc(#loc130)
    %1790 = arith.andi %1779, %ready_128 : i1 loc(#loc130)
    %1791 = arith.ori %1789, %1790 : i1 loc(#loc130)
    %1792 = arith.andi %1788, %1791 : i1 loc(#loc130)
    %1793 = arith.xori %value_127, %true : i1 loc(#loc130)
    %1794 = arith.xori %1773, %true : i1 loc(#loc130)
    %1795 = arith.andi %1779, %1793 : i1 loc(#loc130)
    %1796 = arith.ori %1794, %1795 : i1 loc(#loc130)
    %1797 = arith.andi %1792, %1796 : i1 loc(#loc130)
    %1798 = acir.extract %value_125 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc131)
    acir.path %1773 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc132)
    %1799 = arith.xori %1773, %true : i1 loc(#loc132)
    %1800 = arith.ori %1799, %1779 : i1 loc(#loc132)
    %1801 = arith.andi %1779, %ready_126 : i1 loc(#loc132)
    %1802 = arith.andi %1773, %1801 : i1 loc(#loc132)
    %false_129 = arith.constant {acir.source_type = "bool"} false
    %true_130 = arith.constant {acir.source_type = "bool"} true
    %c0_i32_131 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc133)
    %1803 = arith.cmpi ult, %1798, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc134)
    %1804 = arith.xori %1773, %true : i1 loc(#loc135)
    %1805 = arith.andi %1800, %1802 : i1 loc(#loc135)
    %1806 = arith.ori %1804, %1805 : i1 loc(#loc135)
    %1807 = arith.andi %1797, %1806 : i1 loc(#loc135)
    %1808 = arith.cmpi eq, %1798, %c0_i32_131 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc136)
    %1809 = arith.andi %1802, %1802 : i1
    %1810 = arith.xori %1773, %true : i1
    %1811 = arith.andi %1800, %1802 : i1
    %1812 = arith.ori %1810, %1811 : i1
    %1813 = arith.andi %1807, %1812 : i1
    %1814 = arith.andi %1773, %1808 : i1
    %1815 = arith.andi %1800, %1802 : i1
    %1816 = arith.xori %1808, %true : i1
    %1817 = arith.andi %1773, %1816 : i1
    %1818 = arith.andi %1800, %1802 : i1
    %1819 = arith.xori %1817, %true : i1
    %1820 = arith.ori %1819, %1818 : i1
    %1821 = arith.andi %1818, %1802 : i1
    %1822 = arith.andi %1817, %1821 : i1
    acir.path %1817 {branch = 32 : i64, kind = "branch", txn = 6 : i64} loc(#loc137)
    %value_132, %ready_133 = acir.read(%1817) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc137)
    %1823 = arith.xori %1817, %true : i1 loc(#loc137)
    %1824 = arith.andi %1820, %ready_133 : i1 loc(#loc137)
    %1825 = arith.ori %1823, %1824 : i1 loc(#loc137)
    %1826 = arith.andi %1813, %1825 : i1 loc(#loc137)
    %1827 = arith.andi %ready_133, %1822 : i1 loc(#loc137)
    %1828 = acir.extract %value_132[%1798] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc137)
    %value_134, %ready_135 = acir.read(%1817) {branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc138)
    %1829 = arith.xori %1817, %true : i1 loc(#loc138)
    %1830 = arith.andi %1820, %ready_135 : i1 loc(#loc138)
    %1831 = arith.ori %1829, %1830 : i1 loc(#loc138)
    %1832 = arith.andi %1826, %1831 : i1 loc(#loc138)
    %1833 = arith.andi %ready_135, %1822 : i1 loc(#loc138)
    %1834 = acir.extract %value_134[%1798] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc138)
    %1835 = arith.andi %1827, %1833 : i1 loc(#loc139)
    %1836 = arith.cmpi eq, %1828, %1834 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc139)
    %1837 = arith.andi %1835, %1827 : i1
    %1838 = arith.xori %1817, %true : i1
    %1839 = arith.andi %1820, %1835 : i1
    %1840 = arith.ori %1838, %1839 : i1
    %1841 = arith.andi %1832, %1840 : i1
    %1842 = arith.andi %1817, %1836 : i1
    %1843 = arith.andi %1820, %1835 : i1
    %1844 = arith.xori %1836, %true : i1
    %1845 = arith.andi %1817, %1844 : i1
    %1846 = arith.andi %1820, %1835 : i1
    %1847 = arith.xori %1845, %true : i1
    %1848 = arith.ori %1847, %1846 : i1
    %1849 = arith.andi %1846, %1827 : i1
    %1850 = arith.andi %1845, %1849 : i1
    acir.path %1845 {branch = 33 : i64, kind = "branch", txn = 6 : i64} loc(#loc140)
    acir.path %1845 {branch = 33 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc140)
    %1851 = arith.xori %1845, %true : i1 loc(#loc140)
    %1852 = arith.ori %1851, %1848 : i1 loc(#loc140)
    %1853 = arith.andi %1848, %1850 : i1 loc(#loc140)
    %1854 = arith.andi %1845, %1853 : i1 loc(#loc140)
    %c2_i32_136 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_137 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc141)
    %c3_i32_138 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc142)
    %c1_i32_139 = arith.constant {acir.source_type = "u32"} 1 : i32
    %1855 = arith.subi %1828, %c1_i32_139 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc143)
    %1856 = arith.andi %1855, %c3_i32_138 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc143)
    %1857 = arith.cmpi eq, %1856, %c0_i32_137 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc144)
    %1858 = arith.xori %1845, %true : i1
    %1859 = arith.andi %1852, %1854 : i1
    %1860 = arith.ori %1858, %1859 : i1
    %1861 = arith.andi %1841, %1860 : i1
    %1862 = arith.andi %1845, %1857 : i1
    %1863 = arith.andi %1852, %1854 : i1
    %1864 = arith.xori %1857, %true : i1
    %1865 = arith.andi %1845, %1864 : i1
    %1866 = arith.andi %1852, %1854 : i1
    %1867 = arith.xori %1865, %true : i1
    %1868 = arith.ori %1867, %1866 : i1
    %1869 = arith.cmpi eq, %1856, %c1_i32_139 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc145)
    %1870 = arith.xori %1865, %true : i1
    %1871 = arith.andi %1868, %1854 : i1
    %1872 = arith.ori %1870, %1871 : i1
    %1873 = arith.andi %1861, %1872 : i1
    %1874 = arith.andi %1865, %1869 : i1
    %1875 = arith.andi %1868, %1854 : i1
    %1876 = arith.xori %1869, %true : i1
    %1877 = arith.andi %1865, %1876 : i1
    %1878 = arith.andi %1868, %1854 : i1
    %1879 = arith.xori %1877, %true : i1
    %1880 = arith.ori %1879, %1878 : i1
    %1881 = arith.cmpi eq, %1856, %c2_i32_136 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc146)
    %1882 = arith.xori %1877, %true : i1
    %1883 = arith.andi %1880, %1854 : i1
    %1884 = arith.ori %1882, %1883 : i1
    %1885 = arith.andi %1873, %1884 : i1
    %1886 = arith.andi %1877, %1881 : i1
    %1887 = arith.andi %1880, %1854 : i1
    %1888 = arith.xori %1881, %true : i1
    %1889 = arith.andi %1877, %1888 : i1
    %1890 = arith.andi %1880, %1854 : i1
    %1891 = arith.xori %1889, %true : i1
    %1892 = arith.ori %1891, %1890 : i1
    acir.path %1889 {branch = 34 : i64, kind = "branch", txn = 6 : i64} loc(#loc147)
    acir.path %1889 {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc147)
    %1893 = arith.xori %1889, %true : i1 loc(#loc147)
    %1894 = arith.ori %1893, %1892 : i1 loc(#loc147)
    %value_140, %ready_141 = acir.read(%1889) {branch = 34 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %1895 = arith.xori %1889, %true : i1 loc(#loc148)
    %1896 = arith.andi %1894, %ready_141 : i1 loc(#loc148)
    %1897 = arith.ori %1895, %1896 : i1 loc(#loc148)
    %1898 = arith.andi %1885, %1897 : i1 loc(#loc148)
    %1899 = arith.andi %1894, %ready_141 : i1 loc(#loc149)
    %1900 = arith.andi %1889, %1899 : i1 loc(#loc149)
    %1901 = arith.xori %1886, %true : i1 loc(#loc150)
    %1902 = arith.ori %1901, %1887 : i1 loc(#loc150)
    acir.path %1886 {branch = 35 : i64, kind = "branch", txn = 6 : i64} loc(#loc151)
    acir.path %1886 {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %1903 = arith.xori %1886, %true : i1 loc(#loc151)
    %1904 = arith.ori %1903, %1902 : i1 loc(#loc151)
    %value_142, %ready_143 = acir.read(%1886) {branch = 35 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %1905 = arith.xori %1886, %true : i1 loc(#loc148)
    %1906 = arith.andi %1904, %ready_143 : i1 loc(#loc148)
    %1907 = arith.ori %1905, %1906 : i1 loc(#loc148)
    %1908 = arith.andi %1898, %1907 : i1 loc(#loc148)
    %1909 = arith.andi %1904, %ready_143 : i1 loc(#loc149)
    %1910 = arith.andi %1886, %1909 : i1 loc(#loc149)
    %1911 = arith.xori %1874, %true : i1 loc(#loc152)
    %1912 = arith.ori %1911, %1875 : i1 loc(#loc152)
    acir.path %1874 {branch = 36 : i64, kind = "branch", txn = 6 : i64} loc(#loc153)
    acir.path %1874 {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc153)
    %1913 = arith.xori %1874, %true : i1 loc(#loc153)
    %1914 = arith.ori %1913, %1912 : i1 loc(#loc153)
    %value_144, %ready_145 = acir.read(%1874) {branch = 36 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %1915 = arith.xori %1874, %true : i1 loc(#loc148)
    %1916 = arith.andi %1914, %ready_145 : i1 loc(#loc148)
    %1917 = arith.ori %1915, %1916 : i1 loc(#loc148)
    %1918 = arith.andi %1908, %1917 : i1 loc(#loc148)
    %1919 = arith.andi %1914, %ready_145 : i1 loc(#loc149)
    %1920 = arith.andi %1874, %1919 : i1 loc(#loc149)
    %1921 = arith.xori %1862, %true : i1 loc(#loc154)
    %1922 = arith.ori %1921, %1863 : i1 loc(#loc154)
    acir.path %1862 {branch = 37 : i64, kind = "branch", txn = 6 : i64} loc(#loc155)
    acir.path %1862 {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc155)
    %1923 = arith.xori %1862, %true : i1 loc(#loc155)
    %1924 = arith.ori %1923, %1922 : i1 loc(#loc155)
    %value_146, %ready_147 = acir.read(%1862) {branch = 37 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %1925 = arith.xori %1862, %true : i1 loc(#loc148)
    %1926 = arith.andi %1924, %ready_147 : i1 loc(#loc148)
    %1927 = arith.ori %1925, %1926 : i1 loc(#loc148)
    %1928 = arith.andi %1918, %1927 : i1 loc(#loc148)
    %1929 = arith.andi %1924, %ready_147 : i1 loc(#loc149)
    %1930 = arith.andi %1862, %1929 : i1 loc(#loc149)
    %1931 = arith.andi %1892, %1900 : i1 loc(#loc156)
    %1932 = arith.andi %1889, %1931 : i1 loc(#loc156)
    %1933 = arith.andi %1902, %1910 : i1 loc(#loc156)
    %1934 = arith.andi %1886, %1933 : i1 loc(#loc156)
    %1935 = arith.ori %1932, %1934 : i1 loc(#loc156)
    %1936 = arith.andi %1912, %1920 : i1 loc(#loc156)
    %1937 = arith.andi %1874, %1936 : i1 loc(#loc156)
    %1938 = arith.ori %1935, %1937 : i1 loc(#loc156)
    %1939 = arith.andi %1922, %1930 : i1 loc(#loc156)
    %1940 = arith.andi %1862, %1939 : i1 loc(#loc156)
    %1941 = arith.ori %1938, %1940 : i1 loc(#loc156)
    %1942 = arith.select %1874, %value_144, %value_146 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
    %1943 = arith.select %1886, %value_142, %1942 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
    %1944 = arith.select %1889, %value_140, %1943 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
    %1945 = acir.extract %1944 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc157)
    %1946 = arith.xori %1845, %true : i1
    %1947 = arith.andi %1848, %1941 : i1
    %1948 = arith.ori %1946, %1947 : i1
    %1949 = arith.andi %1928, %1948 : i1
    %1950 = arith.andi %1845, %1945 : i1
    %1951 = arith.andi %1848, %1941 : i1
    %1952 = arith.xori %1945, %true : i1
    %1953 = arith.andi %1845, %1952 : i1
    %1954 = arith.andi %1848, %1941 : i1
    %1955 = arith.xori %1950, %true : i1
    %1956 = arith.ori %1955, %1951 : i1
    %1957 = acir.extract %1944 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc158)
    %1958 = arith.xori %1953, %true : i1
    %1959 = arith.ori %1958, %1954 : i1
    %1960 = arith.ori %1953, %1950 : i1
    %1961 = arith.xori %1950, %true : i1
    %1962 = arith.ori %1961, %1956 : i1
    %1963 = arith.andi %1959, %1962 : i1
    %1964 = arith.andi %1953, %1954 : i1
    %1965 = arith.andi %1956, %1941 : i1
    %1966 = arith.andi %1950, %1965 : i1
    %1967 = arith.ori %1964, %1966 : i1
    %1968 = arith.select %1953, %false_129, %1957 : i1
    %1969 = arith.xori %1845, %true : i1
    %1970 = arith.andi %1848, %1967 : i1
    %1971 = arith.ori %1969, %1970 : i1
    %1972 = arith.andi %1949, %1971 : i1
    %1973 = arith.andi %1845, %1968 : i1
    %1974 = arith.andi %1848, %1967 : i1
    %1975 = arith.xori %1968, %true : i1
    %1976 = arith.andi %1845, %1975 : i1
    %1977 = arith.andi %1848, %1967 : i1
    %1978 = arith.xori %1973, %true : i1
    %1979 = arith.ori %1978, %1974 : i1
    %1980 = acir.extract %1944 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc159)
    %1981 = arith.andi %1941, %1850 : i1 loc(#loc159)
    %1982 = arith.cmpi eq, %1980, %1828 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc159)
    %1983 = arith.xori %1976, %true : i1
    %1984 = arith.ori %1983, %1977 : i1
    %1985 = arith.ori %1976, %1973 : i1
    %1986 = arith.xori %1973, %true : i1
    %1987 = arith.ori %1986, %1979 : i1
    %1988 = arith.andi %1984, %1987 : i1
    %1989 = arith.andi %1976, %1977 : i1
    %1990 = arith.andi %1979, %1981 : i1
    %1991 = arith.andi %1973, %1990 : i1
    %1992 = arith.ori %1989, %1991 : i1
    %1993 = arith.select %1976, %false_129, %1982 : i1
    %1994 = arith.andi %1992, %1850 : i1
    %1995 = arith.xori %1845, %true : i1
    %1996 = arith.andi %1848, %1992 : i1
    %1997 = arith.ori %1995, %1996 : i1
    %1998 = arith.andi %1972, %1997 : i1
    %1999 = arith.andi %1845, %1993 : i1
    %2000 = arith.andi %1848, %1992 : i1
    %2001 = arith.xori %1993, %true : i1
    %2002 = arith.andi %1845, %2001 : i1
    %2003 = arith.andi %1848, %1992 : i1
    %2004 = arith.xori %2002, %true : i1
    %2005 = arith.ori %2004, %2003 : i1
    %2006 = arith.andi %2003, %1850 : i1
    %2007 = arith.andi %2002, %2006 : i1
    %2008 = acir.aggregate(%false_129, %1828, %c0_i32_131) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc160)
    %2009 = arith.xori %1999, %true : i1 loc(#loc161)
    %2010 = arith.ori %2009, %2000 : i1 loc(#loc161)
    %2011 = acir.extract %1944 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %2012 = arith.andi %1850, %1941 : i1 loc(#loc163)
    %2013 = acir.aggregate(%true_130, %1828, %2011) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc163)
    %2014 = arith.xori %1842, %true : i1 loc(#loc164)
    %2015 = arith.ori %2014, %1843 : i1 loc(#loc164)
    acir.path %1842 {branch = 38 : i64, kind = "branch", txn = 6 : i64} loc(#loc165)
    %value_148, %ready_149 = acir.read(%1842) {branch = 38 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc165)
    %2016 = arith.xori %1842, %true : i1 loc(#loc165)
    %2017 = arith.andi %2015, %ready_149 : i1 loc(#loc165)
    %2018 = arith.ori %2016, %2017 : i1 loc(#loc165)
    %2019 = arith.andi %1998, %2018 : i1 loc(#loc165)
    %2020 = arith.andi %ready_149, %1822 : i1 loc(#loc165)
    %2021 = acir.extract %value_148[%1798] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc165)
    %2022 = arith.andi %1827, %2020 : i1 loc(#loc166)
    %2023 = acir.aggregate(%true_130, %1828, %2021) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
    %2024 = arith.xori %1814, %true : i1 loc(#loc167)
    %2025 = arith.ori %2024, %1815 : i1 loc(#loc167)
    %2026 = acir.aggregate(%true_130, %c0_i32_131, %c0_i32_131) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc168)
    %2027 = arith.andi %2005, %2007 : i1 loc(#loc169)
    %2028 = arith.andi %2002, %2027 : i1 loc(#loc169)
    %2029 = arith.andi %2010, %2012 : i1 loc(#loc169)
    %2030 = arith.andi %1999, %2029 : i1 loc(#loc169)
    %2031 = arith.ori %2028, %2030 : i1 loc(#loc169)
    %2032 = arith.andi %2015, %2022 : i1 loc(#loc169)
    %2033 = arith.andi %1842, %2032 : i1 loc(#loc169)
    %2034 = arith.ori %2031, %2033 : i1 loc(#loc169)
    %2035 = arith.andi %1814, %2025 : i1 loc(#loc169)
    %2036 = arith.ori %2034, %2035 : i1 loc(#loc169)
    %2037 = arith.select %1842, %2023, %2026 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
    %2038 = arith.select %1999, %2013, %2037 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
    %2039 = arith.select %2002, %2008, %2038 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
    %2040 = acir.extract %value_125 {acir.id = "v7", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc170)
    acir.path %1773 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc171)
    %2041 = arith.xori %1773, %true : i1 loc(#loc171)
    %2042 = arith.ori %2041, %1779 : i1 loc(#loc171)
    %2043 = arith.andi %1779, %ready_126 : i1 loc(#loc171)
    %2044 = arith.andi %1773, %2043 : i1 loc(#loc171)
    %false_150 = arith.constant {acir.source_type = "bool"} false
    %true_151 = arith.constant {acir.source_type = "bool"} true
    %c0_i32_152 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32_153 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc133)
    %2045 = arith.cmpi ult, %2040, %c8_i32_153 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc134)
    %2046 = arith.xori %1773, %true : i1 loc(#loc135)
    %2047 = arith.andi %2042, %2044 : i1 loc(#loc135)
    %2048 = arith.ori %2046, %2047 : i1 loc(#loc135)
    %2049 = arith.andi %2019, %2048 : i1 loc(#loc135)
    %2050 = arith.cmpi eq, %2040, %c0_i32_152 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc136)
    %2051 = arith.andi %2044, %2044 : i1
    %2052 = arith.xori %1773, %true : i1
    %2053 = arith.andi %2042, %2044 : i1
    %2054 = arith.ori %2052, %2053 : i1
    %2055 = arith.andi %2049, %2054 : i1
    %2056 = arith.andi %1773, %2050 : i1
    %2057 = arith.andi %2042, %2044 : i1
    %2058 = arith.xori %2050, %true : i1
    %2059 = arith.andi %1773, %2058 : i1
    %2060 = arith.andi %2042, %2044 : i1
    %2061 = arith.xori %2059, %true : i1
    %2062 = arith.ori %2061, %2060 : i1
    %2063 = arith.andi %2060, %2044 : i1
    %2064 = arith.andi %2059, %2063 : i1
    acir.path %2059 {branch = 39 : i64, kind = "branch", txn = 6 : i64} loc(#loc137)
    %value_154, %ready_155 = acir.read(%2059) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc137)
    %2065 = arith.xori %2059, %true : i1 loc(#loc137)
    %2066 = arith.andi %2062, %ready_155 : i1 loc(#loc137)
    %2067 = arith.ori %2065, %2066 : i1 loc(#loc137)
    %2068 = arith.andi %2055, %2067 : i1 loc(#loc137)
    %2069 = arith.andi %ready_155, %2064 : i1 loc(#loc137)
    %2070 = acir.extract %value_154[%2040] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc137)
    %value_156, %ready_157 = acir.read(%2059) {branch = 39 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc138)
    %2071 = arith.xori %2059, %true : i1 loc(#loc138)
    %2072 = arith.andi %2062, %ready_157 : i1 loc(#loc138)
    %2073 = arith.ori %2071, %2072 : i1 loc(#loc138)
    %2074 = arith.andi %2068, %2073 : i1 loc(#loc138)
    %2075 = arith.andi %ready_157, %2064 : i1 loc(#loc138)
    %2076 = acir.extract %value_156[%2040] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc138)
    %2077 = arith.andi %2069, %2075 : i1 loc(#loc139)
    %2078 = arith.cmpi eq, %2070, %2076 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc139)
    %2079 = arith.andi %2077, %2069 : i1
    %2080 = arith.xori %2059, %true : i1
    %2081 = arith.andi %2062, %2077 : i1
    %2082 = arith.ori %2080, %2081 : i1
    %2083 = arith.andi %2074, %2082 : i1
    %2084 = arith.andi %2059, %2078 : i1
    %2085 = arith.andi %2062, %2077 : i1
    %2086 = arith.xori %2078, %true : i1
    %2087 = arith.andi %2059, %2086 : i1
    %2088 = arith.andi %2062, %2077 : i1
    %2089 = arith.xori %2087, %true : i1
    %2090 = arith.ori %2089, %2088 : i1
    %2091 = arith.andi %2088, %2069 : i1
    %2092 = arith.andi %2087, %2091 : i1
    acir.path %2087 {branch = 40 : i64, kind = "branch", txn = 6 : i64} loc(#loc140)
    acir.path %2087 {branch = 40 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc140)
    %2093 = arith.xori %2087, %true : i1 loc(#loc140)
    %2094 = arith.ori %2093, %2090 : i1 loc(#loc140)
    %2095 = arith.andi %2090, %2092 : i1 loc(#loc140)
    %2096 = arith.andi %2087, %2095 : i1 loc(#loc140)
    %c2_i32_158 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_159 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc141)
    %c3_i32_160 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc142)
    %c1_i32_161 = arith.constant {acir.source_type = "u32"} 1 : i32
    %2097 = arith.subi %2070, %c1_i32_161 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc143)
    %2098 = arith.andi %2097, %c3_i32_160 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc143)
    %2099 = arith.cmpi eq, %2098, %c0_i32_159 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc144)
    %2100 = arith.xori %2087, %true : i1
    %2101 = arith.andi %2094, %2096 : i1
    %2102 = arith.ori %2100, %2101 : i1
    %2103 = arith.andi %2083, %2102 : i1
    %2104 = arith.andi %2087, %2099 : i1
    %2105 = arith.andi %2094, %2096 : i1
    %2106 = arith.xori %2099, %true : i1
    %2107 = arith.andi %2087, %2106 : i1
    %2108 = arith.andi %2094, %2096 : i1
    %2109 = arith.xori %2107, %true : i1
    %2110 = arith.ori %2109, %2108 : i1
    %2111 = arith.cmpi eq, %2098, %c1_i32_161 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc145)
    %2112 = arith.xori %2107, %true : i1
    %2113 = arith.andi %2110, %2096 : i1
    %2114 = arith.ori %2112, %2113 : i1
    %2115 = arith.andi %2103, %2114 : i1
    %2116 = arith.andi %2107, %2111 : i1
    %2117 = arith.andi %2110, %2096 : i1
    %2118 = arith.xori %2111, %true : i1
    %2119 = arith.andi %2107, %2118 : i1
    %2120 = arith.andi %2110, %2096 : i1
    %2121 = arith.xori %2119, %true : i1
    %2122 = arith.ori %2121, %2120 : i1
    %2123 = arith.cmpi eq, %2098, %c2_i32_158 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc146)
    %2124 = arith.xori %2119, %true : i1
    %2125 = arith.andi %2122, %2096 : i1
    %2126 = arith.ori %2124, %2125 : i1
    %2127 = arith.andi %2115, %2126 : i1
    %2128 = arith.andi %2119, %2123 : i1
    %2129 = arith.andi %2122, %2096 : i1
    %2130 = arith.xori %2123, %true : i1
    %2131 = arith.andi %2119, %2130 : i1
    %2132 = arith.andi %2122, %2096 : i1
    %2133 = arith.xori %2131, %true : i1
    %2134 = arith.ori %2133, %2132 : i1
    acir.path %2131 {branch = 41 : i64, kind = "branch", txn = 6 : i64} loc(#loc147)
    acir.path %2131 {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc147)
    %2135 = arith.xori %2131, %true : i1 loc(#loc147)
    %2136 = arith.ori %2135, %2134 : i1 loc(#loc147)
    %value_162, %ready_163 = acir.read(%2131) {branch = 41 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %2137 = arith.xori %2131, %true : i1 loc(#loc148)
    %2138 = arith.andi %2136, %ready_163 : i1 loc(#loc148)
    %2139 = arith.ori %2137, %2138 : i1 loc(#loc148)
    %2140 = arith.andi %2127, %2139 : i1 loc(#loc148)
    %2141 = arith.andi %2136, %ready_163 : i1 loc(#loc149)
    %2142 = arith.andi %2131, %2141 : i1 loc(#loc149)
    %2143 = arith.xori %2128, %true : i1 loc(#loc150)
    %2144 = arith.ori %2143, %2129 : i1 loc(#loc150)
    acir.path %2128 {branch = 42 : i64, kind = "branch", txn = 6 : i64} loc(#loc151)
    acir.path %2128 {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %2145 = arith.xori %2128, %true : i1 loc(#loc151)
    %2146 = arith.ori %2145, %2144 : i1 loc(#loc151)
    %value_164, %ready_165 = acir.read(%2128) {branch = 42 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %2147 = arith.xori %2128, %true : i1 loc(#loc148)
    %2148 = arith.andi %2146, %ready_165 : i1 loc(#loc148)
    %2149 = arith.ori %2147, %2148 : i1 loc(#loc148)
    %2150 = arith.andi %2140, %2149 : i1 loc(#loc148)
    %2151 = arith.andi %2146, %ready_165 : i1 loc(#loc149)
    %2152 = arith.andi %2128, %2151 : i1 loc(#loc149)
    %2153 = arith.xori %2116, %true : i1 loc(#loc152)
    %2154 = arith.ori %2153, %2117 : i1 loc(#loc152)
    acir.path %2116 {branch = 43 : i64, kind = "branch", txn = 6 : i64} loc(#loc153)
    acir.path %2116 {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc153)
    %2155 = arith.xori %2116, %true : i1 loc(#loc153)
    %2156 = arith.ori %2155, %2154 : i1 loc(#loc153)
    %value_166, %ready_167 = acir.read(%2116) {branch = 43 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %2157 = arith.xori %2116, %true : i1 loc(#loc148)
    %2158 = arith.andi %2156, %ready_167 : i1 loc(#loc148)
    %2159 = arith.ori %2157, %2158 : i1 loc(#loc148)
    %2160 = arith.andi %2150, %2159 : i1 loc(#loc148)
    %2161 = arith.andi %2156, %ready_167 : i1 loc(#loc149)
    %2162 = arith.andi %2116, %2161 : i1 loc(#loc149)
    %2163 = arith.xori %2104, %true : i1 loc(#loc154)
    %2164 = arith.ori %2163, %2105 : i1 loc(#loc154)
    acir.path %2104 {branch = 44 : i64, kind = "branch", txn = 6 : i64} loc(#loc155)
    acir.path %2104 {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc155)
    %2165 = arith.xori %2104, %true : i1 loc(#loc155)
    %2166 = arith.ori %2165, %2164 : i1 loc(#loc155)
    %value_168, %ready_169 = acir.read(%2104) {branch = 44 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob.slot0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc148)
    %2167 = arith.xori %2104, %true : i1 loc(#loc148)
    %2168 = arith.andi %2166, %ready_169 : i1 loc(#loc148)
    %2169 = arith.ori %2167, %2168 : i1 loc(#loc148)
    %2170 = arith.andi %2160, %2169 : i1 loc(#loc148)
    %2171 = arith.andi %2166, %ready_169 : i1 loc(#loc149)
    %2172 = arith.andi %2104, %2171 : i1 loc(#loc149)
    %2173 = arith.andi %2134, %2142 : i1 loc(#loc156)
    %2174 = arith.andi %2131, %2173 : i1 loc(#loc156)
    %2175 = arith.andi %2144, %2152 : i1 loc(#loc156)
    %2176 = arith.andi %2128, %2175 : i1 loc(#loc156)
    %2177 = arith.ori %2174, %2176 : i1 loc(#loc156)
    %2178 = arith.andi %2154, %2162 : i1 loc(#loc156)
    %2179 = arith.andi %2116, %2178 : i1 loc(#loc156)
    %2180 = arith.ori %2177, %2179 : i1 loc(#loc156)
    %2181 = arith.andi %2164, %2172 : i1 loc(#loc156)
    %2182 = arith.andi %2104, %2181 : i1 loc(#loc156)
    %2183 = arith.ori %2180, %2182 : i1 loc(#loc156)
    %2184 = arith.select %2116, %value_166, %value_168 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
    %2185 = arith.select %2128, %value_164, %2184 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
    %2186 = arith.select %2131, %value_162, %2185 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
    %2187 = acir.extract %2186 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc157)
    %2188 = arith.xori %2087, %true : i1
    %2189 = arith.andi %2090, %2183 : i1
    %2190 = arith.ori %2188, %2189 : i1
    %2191 = arith.andi %2170, %2190 : i1
    %2192 = arith.andi %2087, %2187 : i1
    %2193 = arith.andi %2090, %2183 : i1
    %2194 = arith.xori %2187, %true : i1
    %2195 = arith.andi %2087, %2194 : i1
    %2196 = arith.andi %2090, %2183 : i1
    %2197 = arith.xori %2192, %true : i1
    %2198 = arith.ori %2197, %2193 : i1
    %2199 = acir.extract %2186 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc158)
    %2200 = arith.xori %2195, %true : i1
    %2201 = arith.ori %2200, %2196 : i1
    %2202 = arith.ori %2195, %2192 : i1
    %2203 = arith.xori %2192, %true : i1
    %2204 = arith.ori %2203, %2198 : i1
    %2205 = arith.andi %2201, %2204 : i1
    %2206 = arith.andi %2195, %2196 : i1
    %2207 = arith.andi %2198, %2183 : i1
    %2208 = arith.andi %2192, %2207 : i1
    %2209 = arith.ori %2206, %2208 : i1
    %2210 = arith.select %2195, %false_150, %2199 : i1
    %2211 = arith.xori %2087, %true : i1
    %2212 = arith.andi %2090, %2209 : i1
    %2213 = arith.ori %2211, %2212 : i1
    %2214 = arith.andi %2191, %2213 : i1
    %2215 = arith.andi %2087, %2210 : i1
    %2216 = arith.andi %2090, %2209 : i1
    %2217 = arith.xori %2210, %true : i1
    %2218 = arith.andi %2087, %2217 : i1
    %2219 = arith.andi %2090, %2209 : i1
    %2220 = arith.xori %2215, %true : i1
    %2221 = arith.ori %2220, %2216 : i1
    %2222 = acir.extract %2186 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc159)
    %2223 = arith.andi %2183, %2092 : i1 loc(#loc159)
    %2224 = arith.cmpi eq, %2222, %2070 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc159)
    %2225 = arith.xori %2218, %true : i1
    %2226 = arith.ori %2225, %2219 : i1
    %2227 = arith.ori %2218, %2215 : i1
    %2228 = arith.xori %2215, %true : i1
    %2229 = arith.ori %2228, %2221 : i1
    %2230 = arith.andi %2226, %2229 : i1
    %2231 = arith.andi %2218, %2219 : i1
    %2232 = arith.andi %2221, %2223 : i1
    %2233 = arith.andi %2215, %2232 : i1
    %2234 = arith.ori %2231, %2233 : i1
    %2235 = arith.select %2218, %false_150, %2224 : i1
    %2236 = arith.andi %2234, %2092 : i1
    %2237 = arith.xori %2087, %true : i1
    %2238 = arith.andi %2090, %2234 : i1
    %2239 = arith.ori %2237, %2238 : i1
    %2240 = arith.andi %2214, %2239 : i1
    %2241 = arith.andi %2087, %2235 : i1
    %2242 = arith.andi %2090, %2234 : i1
    %2243 = arith.xori %2235, %true : i1
    %2244 = arith.andi %2087, %2243 : i1
    %2245 = arith.andi %2090, %2234 : i1
    %2246 = arith.xori %2244, %true : i1
    %2247 = arith.ori %2246, %2245 : i1
    %2248 = arith.andi %2245, %2092 : i1
    %2249 = arith.andi %2244, %2248 : i1
    %2250 = acir.aggregate(%false_150, %2070, %c0_i32_152) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc160)
    %2251 = arith.xori %2241, %true : i1 loc(#loc161)
    %2252 = arith.ori %2251, %2242 : i1 loc(#loc161)
    %2253 = acir.extract %2186 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %2254 = arith.andi %2092, %2183 : i1 loc(#loc163)
    %2255 = acir.aggregate(%true_151, %2070, %2253) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc163)
    %2256 = arith.xori %2084, %true : i1 loc(#loc164)
    %2257 = arith.ori %2256, %2085 : i1 loc(#loc164)
    acir.path %2084 {branch = 45 : i64, kind = "branch", txn = 6 : i64} loc(#loc165)
    %value_170, %ready_171 = acir.read(%2084) {branch = 45 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc165)
    %2258 = arith.xori %2084, %true : i1 loc(#loc165)
    %2259 = arith.andi %2257, %ready_171 : i1 loc(#loc165)
    %2260 = arith.ori %2258, %2259 : i1 loc(#loc165)
    %2261 = arith.andi %2240, %2260 : i1 loc(#loc165)
    %2262 = arith.andi %ready_171, %2064 : i1 loc(#loc165)
    %2263 = acir.extract %value_170[%2040] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc165)
    %2264 = arith.andi %2069, %2262 : i1 loc(#loc166)
    %2265 = acir.aggregate(%true_151, %2070, %2263) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
    %2266 = arith.xori %2056, %true : i1 loc(#loc167)
    %2267 = arith.ori %2266, %2057 : i1 loc(#loc167)
    %2268 = acir.aggregate(%true_151, %c0_i32_152, %c0_i32_152) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc168)
    %2269 = arith.andi %2247, %2249 : i1 loc(#loc169)
    %2270 = arith.andi %2244, %2269 : i1 loc(#loc169)
    %2271 = arith.andi %2252, %2254 : i1 loc(#loc169)
    %2272 = arith.andi %2241, %2271 : i1 loc(#loc169)
    %2273 = arith.ori %2270, %2272 : i1 loc(#loc169)
    %2274 = arith.andi %2257, %2264 : i1 loc(#loc169)
    %2275 = arith.andi %2084, %2274 : i1 loc(#loc169)
    %2276 = arith.ori %2273, %2275 : i1 loc(#loc169)
    %2277 = arith.andi %2056, %2267 : i1 loc(#loc169)
    %2278 = arith.ori %2276, %2277 : i1 loc(#loc169)
    %2279 = arith.select %2084, %2265, %2268 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
    %2280 = arith.select %2241, %2255, %2279 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
    %2281 = arith.select %2244, %2250, %2280 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
    %2282 = acir.extract %value_125 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc172)
    acir.path %1773 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc173)
    %2283 = arith.xori %1773, %true : i1 loc(#loc173)
    %2284 = arith.ori %2283, %1779 : i1 loc(#loc173)
    %2285 = arith.andi %1779, %ready_126 : i1 loc(#loc173)
    %2286 = arith.andi %1773, %2285 : i1 loc(#loc173)
    %2287 = arith.xori %1773, %true : i1 loc(#loc173)
    %2288 = arith.ori %2287, %2284 : i1 loc(#loc173)
    %2289 = arith.andi %2284, %2286 : i1 loc(#loc173)
    %2290 = arith.andi %1773, %2289 : i1 loc(#loc173)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc174)
    %value_172, %ready_173 = acir.read(%1773) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc175)
    %2291 = arith.xori %1773, %true : i1 loc(#loc175)
    %2292 = arith.andi %2288, %ready_173 : i1 loc(#loc175)
    %2293 = arith.ori %2291, %2292 : i1 loc(#loc175)
    %2294 = arith.andi %2261, %2293 : i1 loc(#loc175)
    %value_174, %ready_175 = acir.read(%1773) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc176)
    %2295 = arith.xori %1773, %true : i1 loc(#loc176)
    %2296 = arith.andi %2288, %ready_175 : i1 loc(#loc176)
    %2297 = arith.ori %2295, %2296 : i1 loc(#loc176)
    %2298 = arith.andi %2294, %2297 : i1 loc(#loc176)
    %2299 = arith.andi %ready_173, %ready_175 : i1 loc(#loc175)
    %2300 = arith.subi %value_172, %value_174 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc175)
    %2301 = arith.cmpi ult, %2300, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc175)
    %2302 = arith.andi %2288, %2299 : i1 loc(#loc177)
    %2303 = arith.andi %1773, %2302 : i1 loc(#loc177)
    %2304 = arith.andi %2284, %2303 : i1 loc(#loc178)
    %2305 = arith.andi %1773, %2304 : i1 loc(#loc178)
    %2306 = arith.andi %2305, %2301 : i1 loc(#loc178)
    %2307 = arith.xori %1773, %true : i1 loc(#loc178)
    %2308 = arith.andi %1779, %2306 : i1 loc(#loc178)
    %2309 = arith.ori %2307, %2308 : i1 loc(#loc178)
    %2310 = arith.andi %2298, %2309 : i1 loc(#loc178)
    %2311 = arith.andi %1779, %2306 : i1 loc(#loc178)
    %2312 = arith.xori %1773, %true : i1 loc(#loc178)
    %2313 = arith.ori %2312, %2311 : i1 loc(#loc178)
    %2314 = arith.andi %2311, %ready_126 : i1 loc(#loc178)
    %2315 = arith.andi %1773, %2314 : i1 loc(#loc178)
    %c2_i32_176 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_177 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc179)
    %c3_i32_178 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc180)
    %c1_i32_179 = arith.constant {acir.source_type = "u32"} 1 : i32
    %value_180, %ready_181 = acir.read(%1773) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc181)
    %2316 = arith.xori %1773, %true : i1 loc(#loc181)
    %2317 = arith.andi %2313, %ready_181 : i1 loc(#loc181)
    %2318 = arith.ori %2316, %2317 : i1 loc(#loc181)
    %2319 = arith.andi %2310, %2318 : i1 loc(#loc181)
    %2320 = arith.subi %value_180, %c1_i32_179 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc182)
    %2321 = arith.andi %2320, %c3_i32_178 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc182)
    %2322 = arith.cmpi eq, %2321, %c0_i32_177 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc183)
    %2323 = arith.xori %1773, %true : i1
    %2324 = arith.andi %2313, %ready_181 : i1
    %2325 = arith.ori %2323, %2324 : i1
    %2326 = arith.andi %2319, %2325 : i1
    %2327 = arith.andi %1773, %2322 : i1
    %2328 = arith.andi %2313, %ready_181 : i1
    %2329 = arith.xori %2322, %true : i1
    %2330 = arith.andi %1773, %2329 : i1
    %2331 = arith.andi %2313, %ready_181 : i1
    %2332 = arith.xori %2330, %true : i1
    %2333 = arith.ori %2332, %2331 : i1
    %2334 = arith.cmpi eq, %2321, %c1_i32_179 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc184)
    %2335 = arith.xori %2330, %true : i1
    %2336 = arith.andi %2333, %ready_181 : i1
    %2337 = arith.ori %2335, %2336 : i1
    %2338 = arith.andi %2326, %2337 : i1
    %2339 = arith.andi %2330, %2334 : i1
    %2340 = arith.andi %2333, %ready_181 : i1
    %2341 = arith.xori %2334, %true : i1
    %2342 = arith.andi %2330, %2341 : i1
    %2343 = arith.andi %2333, %ready_181 : i1
    %2344 = arith.xori %2342, %true : i1
    %2345 = arith.ori %2344, %2343 : i1
    %2346 = arith.cmpi eq, %2321, %c2_i32_176 {acir.id = "v20", acir.source_type = "bool"} : i32 loc(#loc185)
    %2347 = arith.xori %2342, %true : i1
    %2348 = arith.andi %2345, %ready_181 : i1
    %2349 = arith.ori %2347, %2348 : i1
    %2350 = arith.andi %2338, %2349 : i1
    %2351 = arith.andi %2342, %2346 : i1
    %2352 = arith.andi %2345, %ready_181 : i1
    %2353 = arith.xori %2346, %true : i1
    %2354 = arith.andi %2342, %2353 : i1
    %2355 = arith.andi %2345, %ready_181 : i1
    %2356 = arith.xori %2354, %true : i1
    %2357 = arith.ori %2356, %2355 : i1
    %2358 = arith.andi %ready_181, %2315 : i1 loc(#loc186)
    acir.path %2354 {branch = 46 : i64, kind = "branch", txn = 6 : i64} loc(#loc186)
    acir.path %2354 {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc186)
    %2359 = arith.xori %2354, %true : i1 loc(#loc186)
    %2360 = arith.ori %2359, %2357 : i1 loc(#loc186)
    %2361 = arith.andi %2357, %ready_181 : i1 loc(#loc186)
    %2362 = arith.andi %2354, %2361 : i1 loc(#loc186)
    %2363 = arith.andi %2357, %2315 : i1 loc(#loc186)
    %2364 = arith.andi %2354, %2363 : i1 loc(#loc186)
    %2365 = arith.andi %2362, %2364 : i1 loc(#loc186)
    %2366 = arith.xori %2354, %true : i1 loc(#loc186)
    %2367 = arith.ori %2366, %2360 : i1 loc(#loc186)
    %2368 = arith.andi %2360, %2362 : i1 loc(#loc186)
    %2369 = arith.andi %2354, %2368 : i1 loc(#loc186)
    %2370 = arith.andi %2360, %2364 : i1 loc(#loc186)
    %2371 = arith.andi %2354, %2370 : i1 loc(#loc186)
    %value_182, %ready_183 = acir.read(%2354) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc187)
    %2372 = arith.xori %2354, %true : i1 loc(#loc187)
    %2373 = arith.andi %2367, %ready_183 : i1 loc(#loc187)
    %2374 = arith.ori %2372, %2373 : i1 loc(#loc187)
    %2375 = arith.andi %2350, %2374 : i1 loc(#loc187)
    %2376 = acir.extract %value_182 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc187)
    %2377 = acir.unary %2376 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc188)
    %2378 = arith.andi %2367, %ready_183 : i1 loc(#loc189)
    %2379 = arith.andi %2354, %2378 : i1 loc(#loc189)
    %2380 = arith.andi %2360, %2379 : i1 loc(#loc190)
    %2381 = arith.andi %2354, %2380 : i1 loc(#loc190)
    %2382 = arith.andi %2381, %2377 : i1 loc(#loc190)
    %2383 = arith.xori %2354, %true : i1 loc(#loc190)
    %2384 = arith.andi %2357, %2382 : i1 loc(#loc190)
    %2385 = arith.ori %2383, %2384 : i1 loc(#loc190)
    %2386 = arith.andi %2375, %2385 : i1 loc(#loc190)
    %2387 = arith.andi %2357, %2382 : i1 loc(#loc190)
    %2388 = arith.xori %2354, %true : i1 loc(#loc190)
    %2389 = arith.ori %2388, %2387 : i1 loc(#loc190)
    %2390 = arith.andi %2387, %ready_181 : i1 loc(#loc190)
    %2391 = arith.andi %2354, %2390 : i1 loc(#loc190)
    %2392 = arith.andi %2387, %2315 : i1 loc(#loc190)
    %2393 = arith.andi %2354, %2392 : i1 loc(#loc190)
    %true_184 = arith.constant {acir.source_type = "bool"} true loc(#loc191)
    %false_185 = arith.constant {acir.source_type = "bool"} false loc(#loc192)
    %c0_i32_186 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc193)
    %2394 = arith.andi %2391, %2393 : i1 loc(#loc194)
    %2395 = acir.aggregate(%true_184, %false_185, %value_180, %2282, %c0_i32_186) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc194)
    %2396 = arith.xori %2354, %true : i1 loc(#loc195)
    %2397 = arith.andi %2389, %2394 : i1 loc(#loc195)
    %2398 = arith.ori %2396, %2397 : i1 loc(#loc195)
    %2399 = arith.andi %2386, %2398 : i1 loc(#loc195)
    %2400 = arith.andi %2354, %2389 : i1
    %2401 = arith.andi %2400, %2382 : i1
    %2402 = arith.xori %2351, %true : i1
    %2403 = arith.ori %2402, %2352 : i1
    %2404 = arith.andi %ready_181, %2315 : i1 loc(#loc196)
    acir.path %2351 {branch = 47 : i64, kind = "branch", txn = 6 : i64} loc(#loc196)
    acir.path %2351 {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc196)
    %2405 = arith.xori %2351, %true : i1 loc(#loc196)
    %2406 = arith.ori %2405, %2403 : i1 loc(#loc196)
    %2407 = arith.andi %2403, %ready_181 : i1 loc(#loc196)
    %2408 = arith.andi %2351, %2407 : i1 loc(#loc196)
    %2409 = arith.andi %2403, %2315 : i1 loc(#loc196)
    %2410 = arith.andi %2351, %2409 : i1 loc(#loc196)
    %2411 = arith.andi %2408, %2410 : i1 loc(#loc196)
    %2412 = arith.xori %2351, %true : i1 loc(#loc196)
    %2413 = arith.ori %2412, %2406 : i1 loc(#loc196)
    %2414 = arith.andi %2406, %2408 : i1 loc(#loc196)
    %2415 = arith.andi %2351, %2414 : i1 loc(#loc196)
    %2416 = arith.andi %2406, %2410 : i1 loc(#loc196)
    %2417 = arith.andi %2351, %2416 : i1 loc(#loc196)
    %value_187, %ready_188 = acir.read(%2351) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc187)
    %2418 = arith.xori %2351, %true : i1 loc(#loc187)
    %2419 = arith.andi %2413, %ready_188 : i1 loc(#loc187)
    %2420 = arith.ori %2418, %2419 : i1 loc(#loc187)
    %2421 = arith.andi %2399, %2420 : i1 loc(#loc187)
    %2422 = acir.extract %value_187 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc187)
    %2423 = acir.unary %2422 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc188)
    %2424 = arith.andi %2413, %ready_188 : i1 loc(#loc189)
    %2425 = arith.andi %2351, %2424 : i1 loc(#loc189)
    %2426 = arith.andi %2406, %2425 : i1 loc(#loc190)
    %2427 = arith.andi %2351, %2426 : i1 loc(#loc190)
    %2428 = arith.andi %2427, %2423 : i1 loc(#loc190)
    %2429 = arith.xori %2351, %true : i1 loc(#loc190)
    %2430 = arith.andi %2403, %2428 : i1 loc(#loc190)
    %2431 = arith.ori %2429, %2430 : i1 loc(#loc190)
    %2432 = arith.andi %2421, %2431 : i1 loc(#loc190)
    %2433 = arith.andi %2403, %2428 : i1 loc(#loc190)
    %2434 = arith.xori %2351, %true : i1 loc(#loc190)
    %2435 = arith.ori %2434, %2433 : i1 loc(#loc190)
    %2436 = arith.andi %2433, %ready_181 : i1 loc(#loc190)
    %2437 = arith.andi %2351, %2436 : i1 loc(#loc190)
    %2438 = arith.andi %2433, %2315 : i1 loc(#loc190)
    %2439 = arith.andi %2351, %2438 : i1 loc(#loc190)
    %true_189 = arith.constant {acir.source_type = "bool"} true loc(#loc191)
    %false_190 = arith.constant {acir.source_type = "bool"} false loc(#loc192)
    %c0_i32_191 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc193)
    %2440 = arith.andi %2437, %2439 : i1 loc(#loc194)
    %2441 = acir.aggregate(%true_189, %false_190, %value_180, %2282, %c0_i32_191) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc194)
    %2442 = arith.xori %2351, %true : i1 loc(#loc195)
    %2443 = arith.andi %2435, %2440 : i1 loc(#loc195)
    %2444 = arith.ori %2442, %2443 : i1 loc(#loc195)
    %2445 = arith.andi %2432, %2444 : i1 loc(#loc195)
    %2446 = arith.andi %2351, %2435 : i1
    %2447 = arith.andi %2446, %2428 : i1
    %2448 = arith.xori %2354, %true : i1
    %2449 = arith.ori %2448, %2357 : i1
    %2450 = arith.ori %2354, %2351 : i1
    %2451 = arith.xori %2351, %true : i1
    %2452 = arith.ori %2451, %2403 : i1
    %2453 = arith.andi %2449, %2452 : i1
    %2454 = arith.xori %2339, %true : i1
    %2455 = arith.ori %2454, %2340 : i1
    %2456 = arith.andi %ready_181, %2315 : i1 loc(#loc197)
    acir.path %2339 {branch = 48 : i64, kind = "branch", txn = 6 : i64} loc(#loc197)
    acir.path %2339 {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc197)
    %2457 = arith.xori %2339, %true : i1 loc(#loc197)
    %2458 = arith.ori %2457, %2455 : i1 loc(#loc197)
    %2459 = arith.andi %2455, %ready_181 : i1 loc(#loc197)
    %2460 = arith.andi %2339, %2459 : i1 loc(#loc197)
    %2461 = arith.andi %2455, %2315 : i1 loc(#loc197)
    %2462 = arith.andi %2339, %2461 : i1 loc(#loc197)
    %2463 = arith.andi %2460, %2462 : i1 loc(#loc197)
    %2464 = arith.xori %2339, %true : i1 loc(#loc197)
    %2465 = arith.ori %2464, %2458 : i1 loc(#loc197)
    %2466 = arith.andi %2458, %2460 : i1 loc(#loc197)
    %2467 = arith.andi %2339, %2466 : i1 loc(#loc197)
    %2468 = arith.andi %2458, %2462 : i1 loc(#loc197)
    %2469 = arith.andi %2339, %2468 : i1 loc(#loc197)
    %value_192, %ready_193 = acir.read(%2339) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc187)
    %2470 = arith.xori %2339, %true : i1 loc(#loc187)
    %2471 = arith.andi %2465, %ready_193 : i1 loc(#loc187)
    %2472 = arith.ori %2470, %2471 : i1 loc(#loc187)
    %2473 = arith.andi %2445, %2472 : i1 loc(#loc187)
    %2474 = acir.extract %value_192 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc187)
    %2475 = acir.unary %2474 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc188)
    %2476 = arith.andi %2465, %ready_193 : i1 loc(#loc189)
    %2477 = arith.andi %2339, %2476 : i1 loc(#loc189)
    %2478 = arith.andi %2458, %2477 : i1 loc(#loc190)
    %2479 = arith.andi %2339, %2478 : i1 loc(#loc190)
    %2480 = arith.andi %2479, %2475 : i1 loc(#loc190)
    %2481 = arith.xori %2339, %true : i1 loc(#loc190)
    %2482 = arith.andi %2455, %2480 : i1 loc(#loc190)
    %2483 = arith.ori %2481, %2482 : i1 loc(#loc190)
    %2484 = arith.andi %2473, %2483 : i1 loc(#loc190)
    %2485 = arith.andi %2455, %2480 : i1 loc(#loc190)
    %2486 = arith.xori %2339, %true : i1 loc(#loc190)
    %2487 = arith.ori %2486, %2485 : i1 loc(#loc190)
    %2488 = arith.andi %2485, %ready_181 : i1 loc(#loc190)
    %2489 = arith.andi %2339, %2488 : i1 loc(#loc190)
    %2490 = arith.andi %2485, %2315 : i1 loc(#loc190)
    %2491 = arith.andi %2339, %2490 : i1 loc(#loc190)
    %true_194 = arith.constant {acir.source_type = "bool"} true loc(#loc191)
    %false_195 = arith.constant {acir.source_type = "bool"} false loc(#loc192)
    %c0_i32_196 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc193)
    %2492 = arith.andi %2489, %2491 : i1 loc(#loc194)
    %2493 = acir.aggregate(%true_194, %false_195, %value_180, %2282, %c0_i32_196) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc194)
    %2494 = arith.xori %2339, %true : i1 loc(#loc195)
    %2495 = arith.andi %2487, %2492 : i1 loc(#loc195)
    %2496 = arith.ori %2494, %2495 : i1 loc(#loc195)
    %2497 = arith.andi %2484, %2496 : i1 loc(#loc195)
    %2498 = arith.andi %2339, %2487 : i1
    %2499 = arith.andi %2498, %2480 : i1
    %2500 = arith.xori %2342, %true : i1
    %2501 = arith.ori %2500, %2345 : i1
    %2502 = arith.ori %2342, %2339 : i1
    %2503 = arith.xori %2339, %true : i1
    %2504 = arith.ori %2503, %2455 : i1
    %2505 = arith.andi %2501, %2504 : i1
    %2506 = arith.xori %2327, %true : i1
    %2507 = arith.ori %2506, %2328 : i1
    %2508 = arith.andi %ready_181, %2315 : i1 loc(#loc198)
    acir.path %2327 {branch = 49 : i64, kind = "branch", txn = 6 : i64} loc(#loc198)
    acir.path %2327 {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc198)
    %2509 = arith.xori %2327, %true : i1 loc(#loc198)
    %2510 = arith.ori %2509, %2507 : i1 loc(#loc198)
    %2511 = arith.andi %2507, %ready_181 : i1 loc(#loc198)
    %2512 = arith.andi %2327, %2511 : i1 loc(#loc198)
    %2513 = arith.andi %2507, %2315 : i1 loc(#loc198)
    %2514 = arith.andi %2327, %2513 : i1 loc(#loc198)
    %2515 = arith.andi %2512, %2514 : i1 loc(#loc198)
    %2516 = arith.xori %2327, %true : i1 loc(#loc198)
    %2517 = arith.ori %2516, %2510 : i1 loc(#loc198)
    %2518 = arith.andi %2510, %2512 : i1 loc(#loc198)
    %2519 = arith.andi %2327, %2518 : i1 loc(#loc198)
    %2520 = arith.andi %2510, %2514 : i1 loc(#loc198)
    %2521 = arith.andi %2327, %2520 : i1 loc(#loc198)
    %value_197, %ready_198 = acir.read(%2327) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc187)
    %2522 = arith.xori %2327, %true : i1 loc(#loc187)
    %2523 = arith.andi %2517, %ready_198 : i1 loc(#loc187)
    %2524 = arith.ori %2522, %2523 : i1 loc(#loc187)
    %2525 = arith.andi %2497, %2524 : i1 loc(#loc187)
    %2526 = acir.extract %value_197 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc187)
    %2527 = acir.unary %2526 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc188)
    %2528 = arith.andi %2517, %ready_198 : i1 loc(#loc189)
    %2529 = arith.andi %2327, %2528 : i1 loc(#loc189)
    %2530 = arith.andi %2510, %2529 : i1 loc(#loc190)
    %2531 = arith.andi %2327, %2530 : i1 loc(#loc190)
    %2532 = arith.andi %2531, %2527 : i1 loc(#loc190)
    %2533 = arith.xori %2327, %true : i1 loc(#loc190)
    %2534 = arith.andi %2507, %2532 : i1 loc(#loc190)
    %2535 = arith.ori %2533, %2534 : i1 loc(#loc190)
    %2536 = arith.andi %2525, %2535 : i1 loc(#loc190)
    %2537 = arith.andi %2507, %2532 : i1 loc(#loc190)
    %2538 = arith.xori %2327, %true : i1 loc(#loc190)
    %2539 = arith.ori %2538, %2537 : i1 loc(#loc190)
    %2540 = arith.andi %2537, %ready_181 : i1 loc(#loc190)
    %2541 = arith.andi %2327, %2540 : i1 loc(#loc190)
    %2542 = arith.andi %2537, %2315 : i1 loc(#loc190)
    %2543 = arith.andi %2327, %2542 : i1 loc(#loc190)
    %true_199 = arith.constant {acir.source_type = "bool"} true loc(#loc191)
    %false_200 = arith.constant {acir.source_type = "bool"} false loc(#loc192)
    %c0_i32_201 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc193)
    %2544 = arith.andi %2541, %2543 : i1 loc(#loc194)
    %2545 = acir.aggregate(%true_199, %false_200, %value_180, %2282, %c0_i32_201) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc194)
    %2546 = arith.xori %2327, %true : i1 loc(#loc195)
    %2547 = arith.andi %2539, %2544 : i1 loc(#loc195)
    %2548 = arith.ori %2546, %2547 : i1 loc(#loc195)
    %2549 = arith.andi %2536, %2548 : i1 loc(#loc195)
    %2550 = arith.andi %2327, %2539 : i1
    %2551 = arith.andi %2550, %2532 : i1
    %2552 = arith.xori %2330, %true : i1
    %2553 = arith.ori %2552, %2333 : i1
    %2554 = arith.ori %2330, %2327 : i1
    %2555 = arith.xori %2327, %true : i1
    %2556 = arith.ori %2555, %2507 : i1
    %2557 = arith.andi %2553, %2556 : i1
    %2558 = arith.addi %value_180, %c1_i32_179 {acir.id = "v40", acir.source_type = "u32"} : i32 loc(#loc199)
    %2559 = arith.xori %1773, %true : i1 loc(#loc200)
    %2560 = arith.andi %2313, %ready_181 : i1 loc(#loc200)
    %2561 = arith.ori %2559, %2560 : i1 loc(#loc200)
    %2562 = arith.andi %2549, %2561 : i1 loc(#loc200)
    %2563 = arith.andi %2313, %ready_181 : i1 loc(#loc201)
    %2564 = arith.andi %1773, %2563 : i1 loc(#loc201)
    %2565 = arith.andi %2564, %2306 : i1 loc(#loc201)
    %2566 = acir.extract %value_125 {acir.id = "v11", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc202)
    %2567 = arith.andi %ready_126, %2565 : i1 loc(#loc203)
    acir.path %1773 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc203)
    %2568 = arith.xori %1773, %true : i1 loc(#loc203)
    %2569 = arith.ori %2568, %1779 : i1 loc(#loc203)
    %2570 = arith.andi %1779, %ready_126 : i1 loc(#loc203)
    %2571 = arith.andi %1773, %2570 : i1 loc(#loc203)
    %2572 = arith.andi %1779, %2565 : i1 loc(#loc203)
    %2573 = arith.andi %1773, %2572 : i1 loc(#loc203)
    %c0_i32_202 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc204)
    %c8_i32_203 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc205)
    %2574 = arith.cmpi ult, %2566, %c8_i32_203 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc206)
    %2575 = arith.xori %1773, %true : i1 loc(#loc207)
    %2576 = arith.andi %2569, %2571 : i1 loc(#loc207)
    %2577 = arith.ori %2575, %2576 : i1 loc(#loc207)
    %2578 = arith.andi %2562, %2577 : i1 loc(#loc207)
    %2579 = arith.cmpi ne, %2566, %c0_i32_202 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc208)
    %2580 = arith.xori %1773, %true : i1
    %2581 = arith.andi %2569, %2571 : i1
    %2582 = arith.ori %2580, %2581 : i1
    %2583 = arith.andi %2578, %2582 : i1
    %2584 = arith.andi %1773, %2579 : i1
    %2585 = arith.andi %2569, %2571 : i1
    %2586 = arith.xori %2579, %true : i1
    %2587 = arith.andi %1773, %2586 : i1
    %2588 = arith.andi %2569, %2571 : i1
    %2589 = arith.xori %2584, %true : i1
    %2590 = arith.ori %2589, %2585 : i1
    acir.path %2584 {branch = 50 : i64, kind = "branch", txn = 6 : i64} loc(#loc209)
    %value_204, %ready_205 = acir.read(%2584) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc209)
    %2591 = arith.xori %2584, %true : i1 loc(#loc209)
    %2592 = arith.andi %2590, %ready_205 : i1 loc(#loc209)
    %2593 = arith.ori %2591, %2592 : i1 loc(#loc209)
    %2594 = arith.andi %2583, %2593 : i1 loc(#loc209)
    %value_206, %ready_207 = acir.read(%2584) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "read", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc209)
    %2595 = arith.xori %2584, %true : i1 loc(#loc209)
    %2596 = arith.andi %2590, %ready_207 : i1 loc(#loc209)
    %2597 = arith.ori %2595, %2596 : i1 loc(#loc209)
    %2598 = arith.andi %2594, %2597 : i1 loc(#loc209)
    %2599 = arith.andi %ready_207, %2573 : i1 loc(#loc209)
    %2600 = arith.andi %2599, %2571 : i1 loc(#loc209)
    %2601 = acir.update %value_206, %value_180[%2566] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc209)
    %2602 = arith.xori %2584, %true : i1 loc(#loc209)
    %2603 = arith.andi %2590, %2600 : i1 loc(#loc209)
    %2604 = arith.ori %2602, %2603 : i1 loc(#loc209)
    %2605 = arith.andi %2598, %2604 : i1 loc(#loc209)
    %2606 = arith.xori %2587, %true : i1
    %2607 = arith.ori %2606, %2588 : i1
    %2608 = arith.ori %2587, %2584 : i1
    %2609 = arith.xori %2584, %true : i1
    %2610 = arith.ori %2609, %2590 : i1
    %2611 = arith.andi %2607, %2610 : i1
    %2612 = arith.andi %1773, %2569 : i1
    %2613 = acir.extract %value_125 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc210)
    %2614 = acir.extract %value_125 {acir.id = "v15", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc211)
    %2615 = arith.andi %ready_126, %2565 : i1 loc(#loc212)
    %2616 = arith.andi %2615, %ready_126 : i1 loc(#loc212)
    %2617 = arith.andi %2616, %2036 : i1 loc(#loc212)
    %2618 = arith.andi %2617, %2278 : i1 loc(#loc212)
    %2619 = acir.aggregate(%true_124, %2613, %value_180, %2614, %2039, %2281) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc212)
    %value_208, %ready_209 = acir.read(%1773) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc213)
    %2620 = arith.xori %1773, %true : i1 loc(#loc213)
    %2621 = arith.andi %1779, %ready_209 : i1 loc(#loc213)
    %2622 = arith.ori %2620, %2621 : i1 loc(#loc213)
    %2623 = arith.andi %2605, %2622 : i1 loc(#loc213)
    %2624 = acir.extract %value_208 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc213)
    %2625 = acir.unary %2624 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc214)
    %2626 = arith.xori %1773, %true : i1
    %2627 = arith.andi %1779, %ready_209 : i1
    %2628 = arith.ori %2626, %2627 : i1
    %2629 = arith.andi %2623, %2628 : i1
    %2630 = arith.andi %1773, %2625 : i1
    %2631 = arith.andi %1779, %ready_209 : i1
    %2632 = arith.xori %2625, %true : i1
    %2633 = arith.andi %1773, %2632 : i1
    %2634 = arith.andi %1779, %ready_209 : i1
    %2635 = arith.xori %2633, %true : i1
    %2636 = arith.ori %2635, %2634 : i1
    acir.path %2633 {branch = 51 : i64, kind = "branch", txn = 6 : i64} loc(#loc215)
    acir.path %2633 {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc215)
    %2637 = arith.xori %2633, %true : i1 loc(#loc215)
    %2638 = arith.ori %2637, %2636 : i1 loc(#loc215)
    %2639 = arith.andi %2636, %2618 : i1 loc(#loc215)
    %2640 = arith.andi %2633, %2639 : i1 loc(#loc215)
    %2641 = arith.xori %2633, %true : i1 loc(#loc215)
    %2642 = arith.ori %2641, %2638 : i1 loc(#loc215)
    %2643 = arith.andi %2638, %2640 : i1 loc(#loc215)
    %2644 = arith.andi %2633, %2643 : i1 loc(#loc215)
    %value_210, %ready_211 = acir.read(%2633) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc216)
    %2645 = arith.xori %2633, %true : i1 loc(#loc216)
    %2646 = arith.andi %2642, %ready_211 : i1 loc(#loc216)
    %2647 = arith.ori %2645, %2646 : i1 loc(#loc216)
    %2648 = arith.andi %2629, %2647 : i1 loc(#loc216)
    %2649 = acir.extract %value_210 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc216)
    %2650 = acir.unary %2649 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc217)
    %2651 = arith.andi %2642, %ready_211 : i1 loc(#loc218)
    %2652 = arith.andi %2633, %2651 : i1 loc(#loc218)
    %2653 = arith.andi %2638, %2652 : i1 loc(#loc219)
    %2654 = arith.andi %2633, %2653 : i1 loc(#loc219)
    %2655 = arith.andi %2654, %2650 : i1 loc(#loc219)
    %2656 = arith.xori %2633, %true : i1 loc(#loc219)
    %2657 = arith.andi %2636, %2655 : i1 loc(#loc219)
    %2658 = arith.ori %2656, %2657 : i1 loc(#loc219)
    %2659 = arith.andi %2648, %2658 : i1 loc(#loc219)
    %2660 = arith.andi %2636, %2655 : i1 loc(#loc219)
    %2661 = arith.xori %2633, %true : i1 loc(#loc219)
    %2662 = arith.ori %2661, %2660 : i1 loc(#loc219)
    %2663 = arith.andi %2660, %2618 : i1 loc(#loc219)
    %2664 = arith.andi %2633, %2663 : i1 loc(#loc219)
    %2665 = arith.xori %2633, %true : i1 loc(#loc220)
    %2666 = arith.andi %2662, %2664 : i1 loc(#loc220)
    %2667 = arith.ori %2665, %2666 : i1 loc(#loc220)
    %2668 = arith.andi %2659, %2667 : i1 loc(#loc220)
    %2669 = arith.andi %2633, %2662 : i1
    %2670 = arith.andi %2669, %2655 : i1
    %2671 = arith.xori %2630, %true : i1
    %2672 = arith.ori %2671, %2631 : i1
    acir.path %2630 {branch = 52 : i64, kind = "branch", txn = 6 : i64} loc(#loc221)
    acir.path %2630 {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc221)
    %2673 = arith.xori %2630, %true : i1 loc(#loc221)
    %2674 = arith.ori %2673, %2672 : i1 loc(#loc221)
    %2675 = arith.andi %2672, %2618 : i1 loc(#loc221)
    %2676 = arith.andi %2630, %2675 : i1 loc(#loc221)
    %2677 = arith.xori %2630, %true : i1 loc(#loc221)
    %2678 = arith.ori %2677, %2674 : i1 loc(#loc221)
    %2679 = arith.andi %2674, %2676 : i1 loc(#loc221)
    %2680 = arith.andi %2630, %2679 : i1 loc(#loc221)
    %value_212, %ready_213 = acir.read(%2630) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc216)
    %2681 = arith.xori %2630, %true : i1 loc(#loc216)
    %2682 = arith.andi %2678, %ready_213 : i1 loc(#loc216)
    %2683 = arith.ori %2681, %2682 : i1 loc(#loc216)
    %2684 = arith.andi %2668, %2683 : i1 loc(#loc216)
    %2685 = acir.extract %value_212 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc216)
    %2686 = acir.unary %2685 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc217)
    %2687 = arith.andi %2678, %ready_213 : i1 loc(#loc218)
    %2688 = arith.andi %2630, %2687 : i1 loc(#loc218)
    %2689 = arith.andi %2674, %2688 : i1 loc(#loc219)
    %2690 = arith.andi %2630, %2689 : i1 loc(#loc219)
    %2691 = arith.andi %2690, %2686 : i1 loc(#loc219)
    %2692 = arith.xori %2630, %true : i1 loc(#loc219)
    %2693 = arith.andi %2672, %2691 : i1 loc(#loc219)
    %2694 = arith.ori %2692, %2693 : i1 loc(#loc219)
    %2695 = arith.andi %2684, %2694 : i1 loc(#loc219)
    %2696 = arith.andi %2672, %2691 : i1 loc(#loc219)
    %2697 = arith.xori %2630, %true : i1 loc(#loc219)
    %2698 = arith.ori %2697, %2696 : i1 loc(#loc219)
    %2699 = arith.andi %2696, %2618 : i1 loc(#loc219)
    %2700 = arith.andi %2630, %2699 : i1 loc(#loc219)
    %2701 = arith.xori %2630, %true : i1 loc(#loc220)
    %2702 = arith.andi %2698, %2700 : i1 loc(#loc220)
    %2703 = arith.ori %2701, %2702 : i1 loc(#loc220)
    %2704 = arith.andi %2695, %2703 : i1 loc(#loc220)
    %2705 = arith.andi %2630, %2698 : i1
    %2706 = arith.andi %2705, %2691 : i1
    %2707 = arith.xori %2633, %true : i1
    %2708 = arith.ori %2707, %2636 : i1
    %2709 = arith.ori %2633, %2630 : i1
    %2710 = arith.xori %2630, %true : i1
    %2711 = arith.ori %2710, %2672 : i1
    %2712 = arith.andi %2708, %2711 : i1
    %2713 = arith.andi %1773, %1779 : i1
    %2714 = acir.fire "top.dispatch.dispatch#6" %2704 {successors = [], txn = 6 : i64}
    %2715 = arith.andi %2714, %1773 : i1
    acir.effect %2715() {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "pop", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> () loc(#loc130)
    %2716 = arith.andi %2714, %1773 : i1
    acir.effect %2716(%1803) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc135)
    %2717 = arith.andi %2714, %1773 : i1
    acir.effect %2717(%2045) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc135)
    %2718 = arith.andi %2714, %2354 : i1
    acir.effect %2718(%2395) {branch = 46 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot3:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc195)
    %2719 = arith.andi %2714, %2351 : i1
    acir.effect %2719(%2441) {branch = 47 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot2:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc195)
    %2720 = arith.andi %2714, %2339 : i1
    acir.effect %2720(%2493) {branch = 48 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot1:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc195)
    %2721 = arith.andi %2714, %2327 : i1
    acir.effect %2721(%2545) {branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob.slot0:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc195)
    %2722 = arith.andi %2714, %1773 : i1
    acir.effect %2722(%2558) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "write", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1, i32) -> () loc(#loc200)
    %2723 = arith.andi %2714, %1773 : i1
    acir.effect %2723(%2574) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc207)
    %2724 = arith.andi %2714, %2584 : i1
    acir.effect %2724(%2601) {branch = 50 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "write", port = 0 : i64, resource = "top.rf.renamed", txn = 6 : i64} : (i1, !acir.array<i32, 8>) -> () loc(#loc209)
    %2725 = arith.andi %2714, %2633 : i1
    acir.effect %2725(%2619) {branch = 51 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc220)
    %2726 = arith.andi %2714, %2630 : i1
    acir.effect %2726(%2619) {branch = 52 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc220)
    %2727 = arith.xori %1774, %true : i1
    %2728 = arith.ori %2727, %1772 : i1
    %2729 = arith.ori %1774, %1773 : i1
    %2730 = arith.xori %1773, %true : i1
    %2731 = arith.ori %2730, %1776 : i1
    %2732 = arith.andi %2728, %2731 : i1
    acir.path %true {branch = 53 : i64, kind = "branch", txn = 7 : i64} loc(#loc222)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc222)
    %c1_i32_214 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc223)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc224)
    %c2_i32_215 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32_216 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc225)
    %c3_i32_217 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc226)
    %c1_i32_218 = arith.constant {acir.source_type = "u32"} 1 : i32
    %value_219, %ready_220 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc227)
    %2733 = arith.subi %value_219, %c1_i32_218 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc228)
    %2734 = arith.andi %2733, %c3_i32_217 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc228)
    %2735 = arith.cmpi eq, %2734, %c0_i32_216 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc229)
    %2736 = arith.andi %ready_220, %ready_220 : i1
    %2737 = arith.xori %2735, %true : i1
    %2738 = arith.xori %2737, %true : i1
    %2739 = arith.ori %2738, %ready_220 : i1
    %2740 = arith.cmpi eq, %2734, %c1_i32_218 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc230)
    %2741 = arith.xori %2737, %true : i1
    %2742 = arith.andi %2739, %ready_220 : i1
    %2743 = arith.ori %2741, %2742 : i1
    %2744 = arith.andi %2736, %2743 : i1
    %2745 = arith.andi %2737, %2740 : i1
    %2746 = arith.andi %2739, %ready_220 : i1
    %2747 = arith.xori %2740, %true : i1
    %2748 = arith.andi %2737, %2747 : i1
    %2749 = arith.andi %2739, %ready_220 : i1
    %2750 = arith.xori %2748, %true : i1
    %2751 = arith.ori %2750, %2749 : i1
    %2752 = arith.cmpi eq, %2734, %c2_i32_215 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc231)
    %2753 = arith.xori %2748, %true : i1
    %2754 = arith.andi %2751, %ready_220 : i1
    %2755 = arith.ori %2753, %2754 : i1
    %2756 = arith.andi %2744, %2755 : i1
    %2757 = arith.andi %2748, %2752 : i1
    %2758 = arith.andi %2751, %ready_220 : i1
    %2759 = arith.xori %2752, %true : i1
    %2760 = arith.andi %2748, %2759 : i1
    %2761 = arith.andi %2751, %ready_220 : i1
    %2762 = arith.xori %2760, %true : i1
    %2763 = arith.ori %2762, %2761 : i1
    acir.path %2760 {branch = 54 : i64, kind = "branch", txn = 7 : i64} loc(#loc232)
    acir.path %2760 {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc232)
    %2764 = arith.xori %2760, %true : i1 loc(#loc232)
    %2765 = arith.ori %2764, %2763 : i1 loc(#loc232)
    %2766 = arith.andi %2763, %ready_220 : i1 loc(#loc232)
    %2767 = arith.andi %2760, %2766 : i1 loc(#loc232)
    %2768 = arith.xori %2760, %true : i1 loc(#loc232)
    %2769 = arith.ori %2768, %2765 : i1 loc(#loc232)
    %2770 = arith.andi %2765, %2767 : i1 loc(#loc232)
    %2771 = arith.andi %2760, %2770 : i1 loc(#loc232)
    %false_221 = arith.constant {acir.source_type = "bool"} false
    %value_222, %ready_223 = acir.read(%2760) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc233)
    %2772 = arith.xori %2760, %true : i1 loc(#loc233)
    %2773 = arith.andi %2769, %ready_223 : i1 loc(#loc233)
    %2774 = arith.ori %2772, %2773 : i1 loc(#loc233)
    %2775 = arith.andi %2756, %2774 : i1 loc(#loc233)
    %2776 = acir.extract %value_222 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc234)
    %2777 = arith.xori %2760, %true : i1
    %2778 = arith.andi %2769, %ready_223 : i1
    %2779 = arith.ori %2777, %2778 : i1
    %2780 = arith.andi %2775, %2779 : i1
    %2781 = arith.andi %2760, %2776 : i1
    %2782 = arith.andi %2769, %ready_223 : i1
    %2783 = arith.xori %2776, %true : i1
    %2784 = arith.andi %2760, %2783 : i1
    %2785 = arith.andi %2769, %ready_223 : i1
    %2786 = arith.xori %2784, %true : i1
    %2787 = arith.ori %2786, %2785 : i1
    %2788 = arith.xori %2781, %true : i1
    %2789 = arith.ori %2788, %2782 : i1
    %2790 = acir.extract %value_222 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc235)
    %2791 = arith.xori %2784, %true : i1
    %2792 = arith.ori %2791, %2787 : i1
    %2793 = arith.ori %2784, %2781 : i1
    %2794 = arith.xori %2781, %true : i1
    %2795 = arith.ori %2794, %2789 : i1
    %2796 = arith.andi %2792, %2795 : i1
    %2797 = arith.andi %2784, %2787 : i1
    %2798 = arith.andi %2789, %ready_223 : i1
    %2799 = arith.andi %2781, %2798 : i1
    %2800 = arith.ori %2797, %2799 : i1
    %2801 = arith.select %2784, %false_221, %2790 : i1
    %2802 = arith.xori %2760, %true : i1
    %2803 = arith.ori %2802, %2769 : i1
    %2804 = arith.xori %2760, %true : i1
    %2805 = arith.andi %2803, %2800 : i1
    %2806 = arith.ori %2804, %2805 : i1
    %2807 = arith.andi %2780, %2806 : i1
    %2808 = arith.andi %2760, %2801 : i1
    %2809 = arith.andi %2803, %2800 : i1
    %2810 = arith.xori %2801, %true : i1
    %2811 = arith.andi %2760, %2810 : i1
    %2812 = arith.andi %2803, %2800 : i1
    %2813 = arith.xori %2811, %true : i1
    %2814 = arith.ori %2813, %2812 : i1
    %2815 = arith.xori %2808, %true : i1
    %2816 = arith.ori %2815, %2809 : i1
    %2817 = acir.extract %value_222 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc236)
    %2818 = arith.andi %ready_223, %2771 : i1 loc(#loc236)
    %2819 = arith.cmpi eq, %2817, %value_219 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc236)
    %2820 = arith.xori %2811, %true : i1
    %2821 = arith.ori %2820, %2814 : i1
    %2822 = arith.ori %2811, %2808 : i1
    %2823 = arith.xori %2808, %true : i1
    %2824 = arith.ori %2823, %2816 : i1
    %2825 = arith.andi %2821, %2824 : i1
    %2826 = arith.andi %2811, %2814 : i1
    %2827 = arith.andi %2816, %2818 : i1
    %2828 = arith.andi %2808, %2827 : i1
    %2829 = arith.ori %2826, %2828 : i1
    %2830 = arith.select %2811, %false_221, %2819 : i1
    %2831 = arith.xori %2760, %true : i1
    %2832 = arith.ori %2831, %2803 : i1
    %2833 = arith.andi %2832, %2829 : i1 loc(#loc237)
    %2834 = arith.andi %2760, %2833 : i1 loc(#loc237)
    %2835 = arith.andi %2765, %2834 : i1 loc(#loc238)
    %2836 = arith.andi %2760, %2835 : i1 loc(#loc238)
    %2837 = arith.andi %2836, %2830 : i1 loc(#loc238)
    %2838 = arith.xori %2760, %true : i1 loc(#loc238)
    %2839 = arith.andi %2763, %2837 : i1 loc(#loc238)
    %2840 = arith.ori %2838, %2839 : i1 loc(#loc238)
    %2841 = arith.andi %2807, %2840 : i1 loc(#loc238)
    %2842 = arith.andi %2763, %2837 : i1 loc(#loc238)
    %2843 = arith.xori %2760, %true : i1 loc(#loc238)
    %2844 = arith.ori %2843, %2842 : i1 loc(#loc238)
    %2845 = arith.andi %2842, %ready_220 : i1 loc(#loc238)
    %2846 = arith.andi %2760, %2845 : i1 loc(#loc238)
    %c0_i32_224 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_225 = arith.constant {acir.source_type = "bool"} false
    %value_226, %ready_227 = acir.read(%2760) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc239)
    %2847 = arith.xori %2760, %true : i1 loc(#loc239)
    %2848 = arith.andi %2844, %ready_227 : i1 loc(#loc239)
    %2849 = arith.ori %2847, %2848 : i1 loc(#loc239)
    %2850 = arith.andi %2841, %2849 : i1 loc(#loc239)
    %2851 = acir.aggregate(%false_225, %false_225, %c0_i32_224, %c0_i32_224, %c0_i32_224) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc240)
    %2852 = arith.xori %2760, %true : i1 loc(#loc241)
    %2853 = arith.ori %2852, %2844 : i1 loc(#loc241)
    %2854 = arith.andi %2850, %2853 : i1 loc(#loc241)
    %2855 = arith.andi %2844, %ready_227 : i1 loc(#loc242)
    %2856 = arith.andi %2760, %2855 : i1 loc(#loc242)
    %2857 = arith.andi %2856, %2837 : i1 loc(#loc242)
    %2858 = arith.xori %2757, %true : i1
    %2859 = arith.ori %2858, %2758 : i1
    acir.path %2757 {branch = 55 : i64, kind = "branch", txn = 7 : i64} loc(#loc243)
    acir.path %2757 {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc243)
    %2860 = arith.xori %2757, %true : i1 loc(#loc243)
    %2861 = arith.ori %2860, %2859 : i1 loc(#loc243)
    %2862 = arith.andi %2859, %ready_220 : i1 loc(#loc243)
    %2863 = arith.andi %2757, %2862 : i1 loc(#loc243)
    %2864 = arith.xori %2757, %true : i1 loc(#loc243)
    %2865 = arith.ori %2864, %2861 : i1 loc(#loc243)
    %2866 = arith.andi %2861, %2863 : i1 loc(#loc243)
    %2867 = arith.andi %2757, %2866 : i1 loc(#loc243)
    %false_228 = arith.constant {acir.source_type = "bool"} false
    %value_229, %ready_230 = acir.read(%2757) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc233)
    %2868 = arith.xori %2757, %true : i1 loc(#loc233)
    %2869 = arith.andi %2865, %ready_230 : i1 loc(#loc233)
    %2870 = arith.ori %2868, %2869 : i1 loc(#loc233)
    %2871 = arith.andi %2854, %2870 : i1 loc(#loc233)
    %2872 = acir.extract %value_229 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc234)
    %2873 = arith.xori %2757, %true : i1
    %2874 = arith.andi %2865, %ready_230 : i1
    %2875 = arith.ori %2873, %2874 : i1
    %2876 = arith.andi %2871, %2875 : i1
    %2877 = arith.andi %2757, %2872 : i1
    %2878 = arith.andi %2865, %ready_230 : i1
    %2879 = arith.xori %2872, %true : i1
    %2880 = arith.andi %2757, %2879 : i1
    %2881 = arith.andi %2865, %ready_230 : i1
    %2882 = arith.xori %2880, %true : i1
    %2883 = arith.ori %2882, %2881 : i1
    %2884 = arith.xori %2877, %true : i1
    %2885 = arith.ori %2884, %2878 : i1
    %2886 = acir.extract %value_229 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc235)
    %2887 = arith.xori %2880, %true : i1
    %2888 = arith.ori %2887, %2883 : i1
    %2889 = arith.ori %2880, %2877 : i1
    %2890 = arith.xori %2877, %true : i1
    %2891 = arith.ori %2890, %2885 : i1
    %2892 = arith.andi %2888, %2891 : i1
    %2893 = arith.andi %2880, %2883 : i1
    %2894 = arith.andi %2885, %ready_230 : i1
    %2895 = arith.andi %2877, %2894 : i1
    %2896 = arith.ori %2893, %2895 : i1
    %2897 = arith.select %2880, %false_228, %2886 : i1
    %2898 = arith.xori %2757, %true : i1
    %2899 = arith.ori %2898, %2865 : i1
    %2900 = arith.xori %2757, %true : i1
    %2901 = arith.andi %2899, %2896 : i1
    %2902 = arith.ori %2900, %2901 : i1
    %2903 = arith.andi %2876, %2902 : i1
    %2904 = arith.andi %2757, %2897 : i1
    %2905 = arith.andi %2899, %2896 : i1
    %2906 = arith.xori %2897, %true : i1
    %2907 = arith.andi %2757, %2906 : i1
    %2908 = arith.andi %2899, %2896 : i1
    %2909 = arith.xori %2907, %true : i1
    %2910 = arith.ori %2909, %2908 : i1
    %2911 = arith.xori %2904, %true : i1
    %2912 = arith.ori %2911, %2905 : i1
    %2913 = acir.extract %value_229 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc236)
    %2914 = arith.andi %ready_230, %2867 : i1 loc(#loc236)
    %2915 = arith.cmpi eq, %2913, %value_219 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc236)
    %2916 = arith.xori %2907, %true : i1
    %2917 = arith.ori %2916, %2910 : i1
    %2918 = arith.ori %2907, %2904 : i1
    %2919 = arith.xori %2904, %true : i1
    %2920 = arith.ori %2919, %2912 : i1
    %2921 = arith.andi %2917, %2920 : i1
    %2922 = arith.andi %2907, %2910 : i1
    %2923 = arith.andi %2912, %2914 : i1
    %2924 = arith.andi %2904, %2923 : i1
    %2925 = arith.ori %2922, %2924 : i1
    %2926 = arith.select %2907, %false_228, %2915 : i1
    %2927 = arith.xori %2757, %true : i1
    %2928 = arith.ori %2927, %2899 : i1
    %2929 = arith.andi %2928, %2925 : i1 loc(#loc237)
    %2930 = arith.andi %2757, %2929 : i1 loc(#loc237)
    %2931 = arith.andi %2861, %2930 : i1 loc(#loc238)
    %2932 = arith.andi %2757, %2931 : i1 loc(#loc238)
    %2933 = arith.andi %2932, %2926 : i1 loc(#loc238)
    %2934 = arith.xori %2757, %true : i1 loc(#loc238)
    %2935 = arith.andi %2859, %2933 : i1 loc(#loc238)
    %2936 = arith.ori %2934, %2935 : i1 loc(#loc238)
    %2937 = arith.andi %2903, %2936 : i1 loc(#loc238)
    %2938 = arith.andi %2859, %2933 : i1 loc(#loc238)
    %2939 = arith.xori %2757, %true : i1 loc(#loc238)
    %2940 = arith.ori %2939, %2938 : i1 loc(#loc238)
    %2941 = arith.andi %2938, %ready_220 : i1 loc(#loc238)
    %2942 = arith.andi %2757, %2941 : i1 loc(#loc238)
    %c0_i32_231 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_232 = arith.constant {acir.source_type = "bool"} false
    %value_233, %ready_234 = acir.read(%2757) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc239)
    %2943 = arith.xori %2757, %true : i1 loc(#loc239)
    %2944 = arith.andi %2940, %ready_234 : i1 loc(#loc239)
    %2945 = arith.ori %2943, %2944 : i1 loc(#loc239)
    %2946 = arith.andi %2937, %2945 : i1 loc(#loc239)
    %2947 = acir.aggregate(%false_232, %false_232, %c0_i32_231, %c0_i32_231, %c0_i32_231) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc240)
    %2948 = arith.xori %2757, %true : i1 loc(#loc241)
    %2949 = arith.ori %2948, %2940 : i1 loc(#loc241)
    %2950 = arith.andi %2946, %2949 : i1 loc(#loc241)
    %2951 = arith.andi %2940, %ready_234 : i1 loc(#loc242)
    %2952 = arith.andi %2757, %2951 : i1 loc(#loc242)
    %2953 = arith.andi %2952, %2933 : i1 loc(#loc242)
    %2954 = arith.xori %2760, %true : i1
    %2955 = arith.ori %2954, %2763 : i1
    %2956 = arith.ori %2760, %2757 : i1
    %2957 = arith.xori %2757, %true : i1
    %2958 = arith.ori %2957, %2859 : i1
    %2959 = arith.andi %2955, %2958 : i1
    %2960 = arith.andi %2763, %2857 : i1
    %2961 = arith.andi %2760, %2960 : i1
    %2962 = arith.andi %2859, %2953 : i1
    %2963 = arith.andi %2757, %2962 : i1
    %2964 = arith.ori %2961, %2963 : i1
    %2965 = arith.select %2760, %value_226, %value_233 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2966 = arith.xori %2748, %true : i1
    %2967 = arith.ori %2966, %2751 : i1
    %2968 = arith.xori %2745, %true : i1
    %2969 = arith.ori %2968, %2746 : i1
    acir.path %2745 {branch = 56 : i64, kind = "branch", txn = 7 : i64} loc(#loc244)
    acir.path %2745 {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc244)
    %2970 = arith.xori %2745, %true : i1 loc(#loc244)
    %2971 = arith.ori %2970, %2969 : i1 loc(#loc244)
    %2972 = arith.andi %2969, %ready_220 : i1 loc(#loc244)
    %2973 = arith.andi %2745, %2972 : i1 loc(#loc244)
    %2974 = arith.xori %2745, %true : i1 loc(#loc244)
    %2975 = arith.ori %2974, %2971 : i1 loc(#loc244)
    %2976 = arith.andi %2971, %2973 : i1 loc(#loc244)
    %2977 = arith.andi %2745, %2976 : i1 loc(#loc244)
    %false_235 = arith.constant {acir.source_type = "bool"} false
    %value_236, %ready_237 = acir.read(%2745) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc233)
    %2978 = arith.xori %2745, %true : i1 loc(#loc233)
    %2979 = arith.andi %2975, %ready_237 : i1 loc(#loc233)
    %2980 = arith.ori %2978, %2979 : i1 loc(#loc233)
    %2981 = arith.andi %2950, %2980 : i1 loc(#loc233)
    %2982 = acir.extract %value_236 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc234)
    %2983 = arith.xori %2745, %true : i1
    %2984 = arith.andi %2975, %ready_237 : i1
    %2985 = arith.ori %2983, %2984 : i1
    %2986 = arith.andi %2981, %2985 : i1
    %2987 = arith.andi %2745, %2982 : i1
    %2988 = arith.andi %2975, %ready_237 : i1
    %2989 = arith.xori %2982, %true : i1
    %2990 = arith.andi %2745, %2989 : i1
    %2991 = arith.andi %2975, %ready_237 : i1
    %2992 = arith.xori %2990, %true : i1
    %2993 = arith.ori %2992, %2991 : i1
    %2994 = arith.xori %2987, %true : i1
    %2995 = arith.ori %2994, %2988 : i1
    %2996 = acir.extract %value_236 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc235)
    %2997 = arith.xori %2990, %true : i1
    %2998 = arith.ori %2997, %2993 : i1
    %2999 = arith.ori %2990, %2987 : i1
    %3000 = arith.xori %2987, %true : i1
    %3001 = arith.ori %3000, %2995 : i1
    %3002 = arith.andi %2998, %3001 : i1
    %3003 = arith.andi %2990, %2993 : i1
    %3004 = arith.andi %2995, %ready_237 : i1
    %3005 = arith.andi %2987, %3004 : i1
    %3006 = arith.ori %3003, %3005 : i1
    %3007 = arith.select %2990, %false_235, %2996 : i1
    %3008 = arith.xori %2745, %true : i1
    %3009 = arith.ori %3008, %2975 : i1
    %3010 = arith.xori %2745, %true : i1
    %3011 = arith.andi %3009, %3006 : i1
    %3012 = arith.ori %3010, %3011 : i1
    %3013 = arith.andi %2986, %3012 : i1
    %3014 = arith.andi %2745, %3007 : i1
    %3015 = arith.andi %3009, %3006 : i1
    %3016 = arith.xori %3007, %true : i1
    %3017 = arith.andi %2745, %3016 : i1
    %3018 = arith.andi %3009, %3006 : i1
    %3019 = arith.xori %3017, %true : i1
    %3020 = arith.ori %3019, %3018 : i1
    %3021 = arith.xori %3014, %true : i1
    %3022 = arith.ori %3021, %3015 : i1
    %3023 = acir.extract %value_236 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc236)
    %3024 = arith.andi %ready_237, %2977 : i1 loc(#loc236)
    %3025 = arith.cmpi eq, %3023, %value_219 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc236)
    %3026 = arith.xori %3017, %true : i1
    %3027 = arith.ori %3026, %3020 : i1
    %3028 = arith.ori %3017, %3014 : i1
    %3029 = arith.xori %3014, %true : i1
    %3030 = arith.ori %3029, %3022 : i1
    %3031 = arith.andi %3027, %3030 : i1
    %3032 = arith.andi %3017, %3020 : i1
    %3033 = arith.andi %3022, %3024 : i1
    %3034 = arith.andi %3014, %3033 : i1
    %3035 = arith.ori %3032, %3034 : i1
    %3036 = arith.select %3017, %false_235, %3025 : i1
    %3037 = arith.xori %2745, %true : i1
    %3038 = arith.ori %3037, %3009 : i1
    %3039 = arith.andi %3038, %3035 : i1 loc(#loc237)
    %3040 = arith.andi %2745, %3039 : i1 loc(#loc237)
    %3041 = arith.andi %2971, %3040 : i1 loc(#loc238)
    %3042 = arith.andi %2745, %3041 : i1 loc(#loc238)
    %3043 = arith.andi %3042, %3036 : i1 loc(#loc238)
    %3044 = arith.xori %2745, %true : i1 loc(#loc238)
    %3045 = arith.andi %2969, %3043 : i1 loc(#loc238)
    %3046 = arith.ori %3044, %3045 : i1 loc(#loc238)
    %3047 = arith.andi %3013, %3046 : i1 loc(#loc238)
    %3048 = arith.andi %2969, %3043 : i1 loc(#loc238)
    %3049 = arith.xori %2745, %true : i1 loc(#loc238)
    %3050 = arith.ori %3049, %3048 : i1 loc(#loc238)
    %3051 = arith.andi %3048, %ready_220 : i1 loc(#loc238)
    %3052 = arith.andi %2745, %3051 : i1 loc(#loc238)
    %c0_i32_238 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_239 = arith.constant {acir.source_type = "bool"} false
    %value_240, %ready_241 = acir.read(%2745) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc239)
    %3053 = arith.xori %2745, %true : i1 loc(#loc239)
    %3054 = arith.andi %3050, %ready_241 : i1 loc(#loc239)
    %3055 = arith.ori %3053, %3054 : i1 loc(#loc239)
    %3056 = arith.andi %3047, %3055 : i1 loc(#loc239)
    %3057 = acir.aggregate(%false_239, %false_239, %c0_i32_238, %c0_i32_238, %c0_i32_238) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc240)
    %3058 = arith.xori %2745, %true : i1 loc(#loc241)
    %3059 = arith.ori %3058, %3050 : i1 loc(#loc241)
    %3060 = arith.andi %3056, %3059 : i1 loc(#loc241)
    %3061 = arith.andi %3050, %ready_241 : i1 loc(#loc242)
    %3062 = arith.andi %2745, %3061 : i1 loc(#loc242)
    %3063 = arith.andi %3062, %3043 : i1 loc(#loc242)
    %3064 = arith.xori %2748, %true : i1
    %3065 = arith.ori %3064, %2967 : i1
    %3066 = arith.ori %2748, %2745 : i1
    %3067 = arith.xori %2745, %true : i1
    %3068 = arith.ori %3067, %2969 : i1
    %3069 = arith.andi %3065, %3068 : i1
    %3070 = arith.andi %2967, %2964 : i1
    %3071 = arith.andi %2748, %3070 : i1
    %3072 = arith.andi %2969, %3063 : i1
    %3073 = arith.andi %2745, %3072 : i1
    %3074 = arith.ori %3071, %3073 : i1
    %3075 = arith.select %2748, %2965, %value_240 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3076 = arith.xori %2737, %true : i1
    %3077 = arith.ori %3076, %2739 : i1
    %3078 = arith.xori %2735, %true : i1
    %3079 = arith.ori %3078, %ready_220 : i1
    acir.path %2735 {branch = 57 : i64, kind = "branch", txn = 7 : i64} loc(#loc245)
    acir.path %2735 {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc245)
    %3080 = arith.xori %2735, %true : i1 loc(#loc245)
    %3081 = arith.ori %3080, %3079 : i1 loc(#loc245)
    %3082 = arith.andi %3079, %ready_220 : i1 loc(#loc245)
    %3083 = arith.andi %2735, %3082 : i1 loc(#loc245)
    %3084 = arith.xori %2735, %true : i1 loc(#loc245)
    %3085 = arith.ori %3084, %3081 : i1 loc(#loc245)
    %3086 = arith.andi %3081, %3083 : i1 loc(#loc245)
    %3087 = arith.andi %2735, %3086 : i1 loc(#loc245)
    %false_242 = arith.constant {acir.source_type = "bool"} false
    %value_243, %ready_244 = acir.read(%2735) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc233)
    %3088 = arith.xori %2735, %true : i1 loc(#loc233)
    %3089 = arith.andi %3085, %ready_244 : i1 loc(#loc233)
    %3090 = arith.ori %3088, %3089 : i1 loc(#loc233)
    %3091 = arith.andi %3060, %3090 : i1 loc(#loc233)
    %3092 = acir.extract %value_243 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc234)
    %3093 = arith.xori %2735, %true : i1
    %3094 = arith.andi %3085, %ready_244 : i1
    %3095 = arith.ori %3093, %3094 : i1
    %3096 = arith.andi %3091, %3095 : i1
    %3097 = arith.andi %2735, %3092 : i1
    %3098 = arith.andi %3085, %ready_244 : i1
    %3099 = arith.xori %3092, %true : i1
    %3100 = arith.andi %2735, %3099 : i1
    %3101 = arith.andi %3085, %ready_244 : i1
    %3102 = arith.xori %3100, %true : i1
    %3103 = arith.ori %3102, %3101 : i1
    %3104 = arith.xori %3097, %true : i1
    %3105 = arith.ori %3104, %3098 : i1
    %3106 = acir.extract %value_243 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc235)
    %3107 = arith.xori %3100, %true : i1
    %3108 = arith.ori %3107, %3103 : i1
    %3109 = arith.ori %3100, %3097 : i1
    %3110 = arith.xori %3097, %true : i1
    %3111 = arith.ori %3110, %3105 : i1
    %3112 = arith.andi %3108, %3111 : i1
    %3113 = arith.andi %3100, %3103 : i1
    %3114 = arith.andi %3105, %ready_244 : i1
    %3115 = arith.andi %3097, %3114 : i1
    %3116 = arith.ori %3113, %3115 : i1
    %3117 = arith.select %3100, %false_242, %3106 : i1
    %3118 = arith.xori %2735, %true : i1
    %3119 = arith.ori %3118, %3085 : i1
    %3120 = arith.xori %2735, %true : i1
    %3121 = arith.andi %3119, %3116 : i1
    %3122 = arith.ori %3120, %3121 : i1
    %3123 = arith.andi %3096, %3122 : i1
    %3124 = arith.andi %2735, %3117 : i1
    %3125 = arith.andi %3119, %3116 : i1
    %3126 = arith.xori %3117, %true : i1
    %3127 = arith.andi %2735, %3126 : i1
    %3128 = arith.andi %3119, %3116 : i1
    %3129 = arith.xori %3127, %true : i1
    %3130 = arith.ori %3129, %3128 : i1
    %3131 = arith.xori %3124, %true : i1
    %3132 = arith.ori %3131, %3125 : i1
    %3133 = acir.extract %value_243 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc236)
    %3134 = arith.andi %ready_244, %3087 : i1 loc(#loc236)
    %3135 = arith.cmpi eq, %3133, %value_219 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc236)
    %3136 = arith.xori %3127, %true : i1
    %3137 = arith.ori %3136, %3130 : i1
    %3138 = arith.ori %3127, %3124 : i1
    %3139 = arith.xori %3124, %true : i1
    %3140 = arith.ori %3139, %3132 : i1
    %3141 = arith.andi %3137, %3140 : i1
    %3142 = arith.andi %3127, %3130 : i1
    %3143 = arith.andi %3132, %3134 : i1
    %3144 = arith.andi %3124, %3143 : i1
    %3145 = arith.ori %3142, %3144 : i1
    %3146 = arith.select %3127, %false_242, %3135 : i1
    %3147 = arith.xori %2735, %true : i1
    %3148 = arith.ori %3147, %3119 : i1
    %3149 = arith.andi %3148, %3145 : i1 loc(#loc237)
    %3150 = arith.andi %2735, %3149 : i1 loc(#loc237)
    %3151 = arith.andi %3081, %3150 : i1 loc(#loc238)
    %3152 = arith.andi %2735, %3151 : i1 loc(#loc238)
    %3153 = arith.andi %3152, %3146 : i1 loc(#loc238)
    %3154 = arith.xori %2735, %true : i1 loc(#loc238)
    %3155 = arith.andi %3079, %3153 : i1 loc(#loc238)
    %3156 = arith.ori %3154, %3155 : i1 loc(#loc238)
    %3157 = arith.andi %3123, %3156 : i1 loc(#loc238)
    %3158 = arith.andi %3079, %3153 : i1 loc(#loc238)
    %3159 = arith.xori %2735, %true : i1 loc(#loc238)
    %3160 = arith.ori %3159, %3158 : i1 loc(#loc238)
    %3161 = arith.andi %3158, %ready_220 : i1 loc(#loc238)
    %3162 = arith.andi %2735, %3161 : i1 loc(#loc238)
    %c0_i32_245 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false_246 = arith.constant {acir.source_type = "bool"} false
    %value_247, %ready_248 = acir.read(%2735) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "read", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc239)
    %3163 = arith.xori %2735, %true : i1 loc(#loc239)
    %3164 = arith.andi %3160, %ready_248 : i1 loc(#loc239)
    %3165 = arith.ori %3163, %3164 : i1 loc(#loc239)
    %3166 = arith.andi %3157, %3165 : i1 loc(#loc239)
    %3167 = acir.aggregate(%false_246, %false_246, %c0_i32_245, %c0_i32_245, %c0_i32_245) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc240)
    %3168 = arith.xori %2735, %true : i1 loc(#loc241)
    %3169 = arith.ori %3168, %3160 : i1 loc(#loc241)
    %3170 = arith.andi %3166, %3169 : i1 loc(#loc241)
    %3171 = arith.andi %3160, %ready_248 : i1 loc(#loc242)
    %3172 = arith.andi %2735, %3171 : i1 loc(#loc242)
    %3173 = arith.andi %3172, %3153 : i1 loc(#loc242)
    %3174 = arith.xori %2737, %true : i1
    %3175 = arith.ori %3174, %3077 : i1
    %3176 = arith.ori %2737, %2735 : i1
    %3177 = arith.xori %2735, %true : i1
    %3178 = arith.ori %3177, %3079 : i1
    %3179 = arith.andi %3175, %3178 : i1
    %3180 = arith.andi %3077, %3074 : i1
    %3181 = arith.andi %2737, %3180 : i1
    %3182 = arith.andi %3079, %3173 : i1
    %3183 = arith.andi %2735, %3182 : i1
    %3184 = arith.ori %3181, %3183 : i1
    %3185 = arith.select %2737, %3075, %value_247 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3186 = arith.addi %value_219, %c1_i32_218 {acir.id = "v38", acir.source_type = "u32"} : i32 loc(#loc246)
    %3187 = arith.andi %3170, %ready_220 : i1 loc(#loc247)
    acir.path %true {branch = 53 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc248)
    %c0_i32_249 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc249)
    %3188 = acir.extract %3185 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc250)
    %3189 = arith.cmpi ne, %3188, %c0_i32_249 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc250)
    %3190 = arith.andi %3187, %3184 : i1
    %3191 = arith.xori %3189, %true : i1
    %3192 = arith.xori %3189, %true : i1
    %3193 = arith.ori %3192, %3184 : i1
    %3194 = acir.extract %3185 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc251)
    acir.path %3189 {branch = 58 : i64, kind = "branch", txn = 7 : i64} loc(#loc252)
    %value_250, %ready_251 = acir.read(%3189) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc252)
    %3195 = arith.xori %3189, %true : i1 loc(#loc252)
    %3196 = arith.andi %3193, %ready_251 : i1 loc(#loc252)
    %3197 = arith.ori %3195, %3196 : i1 loc(#loc252)
    %3198 = arith.andi %3190, %3197 : i1 loc(#loc252)
    %value_252, %ready_253 = acir.read(%3189) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc252)
    %3199 = arith.xori %3189, %true : i1 loc(#loc252)
    %3200 = arith.andi %3193, %ready_253 : i1 loc(#loc252)
    %3201 = arith.ori %3199, %3200 : i1 loc(#loc252)
    %3202 = arith.andi %3198, %3201 : i1 loc(#loc252)
    %3203 = acir.extract %3185 {acir.id = "v17", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc253)
    %3204 = arith.andi %ready_253, %3184 : i1 loc(#loc252)
    %3205 = arith.andi %3204, %3184 : i1 loc(#loc252)
    %3206 = acir.update %value_252, %3194[%3203] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc252)
    %3207 = arith.xori %3189, %true : i1 loc(#loc252)
    %3208 = arith.andi %3193, %3205 : i1 loc(#loc252)
    %3209 = arith.ori %3207, %3208 : i1 loc(#loc252)
    %3210 = arith.andi %3202, %3209 : i1 loc(#loc252)
    %3211 = acir.extract %3185 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc254)
    %value_254, %ready_255 = acir.read(%3189) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc255)
    %3212 = arith.xori %3189, %true : i1 loc(#loc255)
    %3213 = arith.andi %3193, %ready_255 : i1 loc(#loc255)
    %3214 = arith.ori %3212, %3213 : i1 loc(#loc255)
    %3215 = arith.andi %3210, %3214 : i1 loc(#loc255)
    %value_256, %ready_257 = acir.read(%3189) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1) -> (!acir.array<i32, 8>, i1) loc(#loc255)
    %3216 = arith.xori %3189, %true : i1 loc(#loc255)
    %3217 = arith.andi %3193, %ready_257 : i1 loc(#loc255)
    %3218 = arith.ori %3216, %3217 : i1 loc(#loc255)
    %3219 = arith.andi %3215, %3218 : i1 loc(#loc255)
    %3220 = acir.extract %3185 {acir.id = "v30", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc256)
    %3221 = arith.andi %ready_257, %3184 : i1 loc(#loc255)
    %3222 = arith.andi %3221, %3184 : i1 loc(#loc255)
    %3223 = acir.update %value_256, %3211[%3220] {acir.id = "v31", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc255)
    %3224 = arith.xori %3189, %true : i1 loc(#loc255)
    %3225 = arith.andi %3193, %3222 : i1 loc(#loc255)
    %3226 = arith.ori %3224, %3225 : i1 loc(#loc255)
    %3227 = arith.andi %3219, %3226 : i1 loc(#loc255)
    %3228 = arith.xori %3191, %true : i1
    %3229 = arith.ori %3228, %3184 : i1
    %3230 = arith.ori %3191, %3189 : i1
    %3231 = arith.xori %3189, %true : i1
    %3232 = arith.ori %3231, %3193 : i1
    %3233 = arith.andi %3229, %3232 : i1
    %value_258, %ready_259 = acir.read(%true) {branch = 53 : i64, frames = ["top.commit:commit"], method = "read", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc257)
    %3234 = arith.andi %3227, %ready_259 : i1 loc(#loc257)
    %3235 = arith.addi %value_258, %c1_i32_214 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc257)
    %3236 = arith.andi %3234, %ready_259 : i1 loc(#loc258)
    %3237 = acir.extract %3185 {acir.id = "v11", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc259)
    %3238 = arith.andi %3236, %3184 : i1 loc(#loc260)
    %3239 = acir.fire "top.commit.commit#7" %3238 {successors = [], txn = 7 : i64}
    %3240 = arith.andi %3239, %2760 : i1
    acir.effect %3240(%2851) {branch = 54 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot3:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot3.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc241)
    %3241 = arith.andi %3239, %2757 : i1
    acir.effect %3241(%2947) {branch = 55 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot2:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot2.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc241)
    %3242 = arith.andi %3239, %2745 : i1
    acir.effect %3242(%3057) {branch = 56 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot1:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot1.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc241)
    %3243 = arith.andi %3239, %2735 : i1
    acir.effect %3243(%3167) {branch = 57 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob.slot0:retire"], method = "write", port = 0 : i64, resource = "top.rob.slot0.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc241)
    acir.effect %3239(%3186) {branch = 53 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "write", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1, i32) -> () loc(#loc247)
    %3244 = arith.andi %3239, %3189 : i1
    acir.effect %3244(%3206) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.registers", txn = 7 : i64} : (i1, !acir.array<i32, 8>) -> () loc(#loc252)
    %3245 = arith.andi %3239, %3189 : i1
    acir.effect %3245(%3223) {branch = 58 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.committed", txn = 7 : i64} : (i1, !acir.array<i32, 8>) -> () loc(#loc255)
    acir.effect %3239(%3235) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1, i32) -> () loc(#loc258)
    acir.effect %3239(%3237) {branch = 53 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.last_tag", txn = 7 : i64} : (i1, i32) -> () loc(#loc260)
    acir.path %true {branch = 59 : i64, kind = "branch", txn = 8 : i64} loc(#loc261)
    acir.path %true {branch = 59 : i64, frames = ["top.audit:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 8 : i64} loc(#loc261)
    %value_260, %ready_261 = acir.read(%true) {branch = 59 : i64, frames = ["top.audit:observe"], method = "get", port = 0 : i64, resource = "top.cdb.event", txn = 8 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) loc(#loc262)
    %3246 = acir.extract %value_260 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc263)
    %3247 = arith.andi %ready_261, %ready_261 : i1 loc(#loc264)
    %3248 = acir.fire "top.audit.observe#8" %3247 {successors = [], txn = 8 : i64}
    acir.effect %3248(%3246) {branch = 59 : i64, frames = ["top.audit:observe"], method = "write", port = 0 : i64, resource = "top.audit.last_tag", txn = 8 : i64} : (i1, i32) -> () loc(#loc264)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":109:9)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":110:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":112:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":113:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":114:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":117:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":155:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":157:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":158:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:13)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:13)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:19)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:19)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:31)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:47)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:29)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:38)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:51)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:59)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:38)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:51)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:59)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":36:24)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":17:16)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:17)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:16)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:32)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:55)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:75)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:9)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:20)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:16)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:17)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:9)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:9)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":17:9)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:20)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:13)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":37:24)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":23:16)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":23:9)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:20)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:13)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":52:15)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":48:17)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:27)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:38)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:48)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:66)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:9)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":80:16)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":84:13)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:18)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:21)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":66:19)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":66:9)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:9)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:9)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":82:19)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:25)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:12)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:17)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":76:9)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:27)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:38)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:48)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:66)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:16)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:9)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:25)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:13)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":74:13)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:17)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:22)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:21)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:27)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:17)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:12)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:14)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:14)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:13)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:12)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:33)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:29)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:50)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:13)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":63:13)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:13)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:13)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:13)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":131:13)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":30:17)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:12)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:20)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:16)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:40)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:29)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:17)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:20)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:16)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:40)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:29)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:17)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":40:13)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":49:9)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:23)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:24)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:16)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:9)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":172:12)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:15)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:19)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:12)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":177:17)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:21)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:27)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:17)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:12)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:14)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:14)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:16)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:16)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:9)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:9)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:20)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:13)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:20)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:13)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:20)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:13)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:12)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:28)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:44)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:16)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:9)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:39)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:20)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:13)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:39)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:20)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:13)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":173:20)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":173:13)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:30)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:16)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:23)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:9)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:20)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:21)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:27)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:15)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:17)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:12)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:14)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":109:14)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:13)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:20)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:16)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:9)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:20)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:26)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:32)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:48)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:17)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:9)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:13)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:13)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:13)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:16)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:9)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:9)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":185:18)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:21)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:16)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:9)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":185:12)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":186:13)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:20)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:16)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:9)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":14:20)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":16:9)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":71:9)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:29)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:17)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:21)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:27)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:15)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:17)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:12)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:14)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:14)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":150:21)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":71:17)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:32)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:48)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:9)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":74:20)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:17)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:17)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:9)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":78:9)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:21)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:21)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:21)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":151:16)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":151:9)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":190:24)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":190:12)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:35)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:13)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:23)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:35)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:13)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:23)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:19)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:20)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:9)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":86:9)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":82:17)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":83:20)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":83:9)

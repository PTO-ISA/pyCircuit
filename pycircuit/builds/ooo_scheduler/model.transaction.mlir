module attributes {acir.instances = [{definition = "src_model_3cf90884.OutOfOrder", name = "top", parent, rules = []}, {definition = "src_state_7e811fd9.ReorderBuffer", name = "top.rob", parent = "top", rules = ["allocate", "complete", "has_room", "lookup", "retire", "room"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.0", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.1", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.2", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.3", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RegisterFile", name = "top.rf", parent = "top", rules = ["commit", "operand", "rename"]}, {definition = "src_state_7e811fd9.ReservationStation", name = "top.left", parent = "top", rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}, {definition = "src_state_7e811fd9.ReservationStation", name = "top.right", parent = "top", rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}, {definition = "src_execution_885f3671.Scheduler", name = "top.scheduler", parent = "top", rules = ["issue_add", "issue_mul"]}, {definition = "src_execution_885f3671.Adder", name = "top.adder", parent = "top", rules = ["execute"]}, {definition = "src_execution_885f3671.Multiplier", name = "top.multiplier", parent = "top", rules = ["accept", "advance"]}, {definition = "src_model_3cf90884.Broadcast", name = "top.cdb", parent = "top", rules = ["broadcast"]}, {definition = "src_model_3cf90884.Dispatch", name = "top.dispatch", parent = "top", rules = ["dispatch"]}, {definition = "src_model_3cf90884.Commit", name = "top.commit", parent = "top", rules = ["commit"]}, {definition = "src_model_3cf90884.CompletionAudit", name = "top.audit", parent = "top", rules = ["observe"]}], acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, format_version = 9 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default, name = "lhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "done", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}, acir.stage = "transaction"} {
  %c4_i32 = arith.constant 4 : i32
  %c7_i64 = arith.constant 7 : i64
  %c6_i64 = arith.constant 6 : i64
  %c5_i64 = arith.constant 5 : i64
  %c4_i64 = arith.constant 4 : i64
  %c3_i64 = arith.constant 3 : i64
  %c2_i64 = arith.constant 2 : i64
  %c0_i64 = arith.constant 0 : i64
  %c1_i64 = arith.constant 1 : i64
  %c8_i64 = arith.constant 8 : i64
  %c8_i32 = arith.constant 8 : i32
  %c3_i32 = arith.constant 3 : i32
  %c2_i32 = arith.constant 2 : i32
  %c1_i32 = arith.constant 1 : i32
  %c0_i32 = arith.constant 0 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 5 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.event", source_branch = 29 : i64, target_branches = [131], to = 8 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}], schedule = [3, 1, 5, 2, 0, 4, 6, 7, 8], stage = "transaction"} {
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
    acir.state @top.rf.committed.0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.5 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.6 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.7 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.2 : i32 {initial = 6 : i32, kind = "cell"}
    acir.state @top.rf.registers.3 : i32 {initial = 7 : i32, kind = "cell"}
    acir.state @top.rf.registers.4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.5 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.6 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.7 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.5 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.6 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.7 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]], kind = "cell"}
    acir.state @top.rob.head : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.rob.slots.0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slots.1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slots.2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.slots.3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rob.tail : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity = 4 : i64, initial = [[1 : i32, 1 : i32, 2 : i32, 3 : i32], [0 : i32, 4 : i32, 1 : i32, 2 : i32], [0 : i32, 5 : i32, 2 : i32, 3 : i32], [0 : i32, 6 : i32, 4 : i32, 5 : i32]], kind = "queue"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64}
    acir.path %true {branch = 0 : i64, frames = ["top.left:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.left:peek"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc1)
    %0 = acir.export %value : !acir.struct<"src_payloads_25ebcaa7.Entry"> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc2)
    %1 = acir.export %true : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.right:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = ["top.right:peek"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = -1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc1)
    %2 = acir.export %value_0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc3)
    %3 = acir.export %true : i1 -> i1
    %4 = acir.extract %0 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc4)
    %5 = acir.extract %0 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc5)
    %6 = acir.extract %5 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc5)
    %7 = arith.andi %4, %6 : i1
    %8 = arith.xori %4, %true : i1
    %9 = arith.ori %8, %1 : i1
    %10 = arith.andi %1, %9 : i1
    %11 = acir.extract %0 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc6)
    %12 = acir.extract %11 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc6)
    %13 = arith.andi %7, %12 : i1 loc(#loc7)
    %14 = arith.xori %7, %true : i1
    %15 = arith.ori %14, %1 : i1
    %16 = arith.andi %10, %15 : i1
    %17 = acir.extract %2 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc8)
    %18 = acir.extract %2 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc9)
    %19 = acir.extract %18 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc9)
    %20 = arith.andi %17, %19 : i1
    %21 = arith.xori %17, %true : i1
    %22 = arith.ori %21, %3 : i1
    %23 = arith.andi %3, %22 : i1
    %24 = acir.extract %2 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc10)
    %25 = acir.extract %24 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc10)
    %26 = arith.andi %20, %25 : i1 loc(#loc11)
    %27 = arith.xori %20, %true : i1
    %28 = arith.ori %27, %3 : i1
    %29 = arith.andi %23, %28 : i1
    %30 = acir.extract %0 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc12)
    %31 = arith.cmpi eq, %30, %c0_i32 : i32 loc(#loc12)
    %32 = arith.andi %13, %31 : i1 loc(#loc13)
    %33 = arith.xori %13, %true : i1
    %34 = arith.ori %33, %1 : i1
    %35 = arith.andi %16, %34 : i1
    %36 = acir.extract %2 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc14)
    %37 = arith.cmpi eq, %36, %c0_i32 : i32 loc(#loc14)
    %38 = arith.andi %26, %37 : i1 loc(#loc15)
    %39 = arith.xori %26, %true : i1
    %40 = arith.ori %39, %3 : i1
    %41 = arith.andi %29, %40 : i1
    %42 = arith.cmpi eq, %30, %c1_i32 : i32 loc(#loc16)
    %43 = arith.andi %13, %42 : i1 loc(#loc17)
    %44 = arith.cmpi eq, %36, %c1_i32 : i32 loc(#loc18)
    %45 = arith.andi %26, %44 : i1 loc(#loc19)
    %46 = arith.xori %38, %true : i1 loc(#loc20)
    %47 = acir.extract %0 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc21)
    %48 = acir.extract %2 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc22)
    %49 = arith.andi %1, %3 : i1 loc(#loc21)
    %50 = arith.cmpi ult, %47, %48 : i32 loc(#loc21)
    %51 = arith.ori %46, %50 : i1
    %52 = arith.ori %46, %49 : i1
    %53 = arith.andi %41, %52 : i1
    %54 = arith.andi %32, %51 : i1 loc(#loc23)
    %55 = arith.xori %32, %true : i1
    %56 = arith.ori %55, %53 : i1
    %57 = arith.andi %35, %56 : i1
    %58 = arith.xori %45, %true : i1 loc(#loc24)
    %59 = arith.ori %58, %50 : i1
    %60 = arith.ori %58, %49 : i1
    %61 = arith.andi %41, %60 : i1
    %62 = arith.andi %43, %59 : i1 loc(#loc25)
    %63 = arith.xori %43, %true : i1
    %64 = arith.ori %63, %61 : i1
    %65 = arith.andi %35, %64 : i1
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc26)
    acir.path %true {branch = 1 : i64, frames = ["top.scheduler:issue_add"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc26)
    %66 = arith.xori %54, %true : i1 loc(#loc27)
    acir.path %66 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc28)
    acir.path %66 {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc28)
    acir.path %66 {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take", "top.right:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc28)
    %value_2, %ready_3 = acir.read(%66) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take", "top.right:issuable"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %67 = acir.bind %value_2 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc29)
    %68 = acir.extract %67 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc30)
    %69 = acir.extract %67 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc31)
    %70 = arith.cmpi eq, %69, %c0_i32 : i32 loc(#loc31)
    %71 = arith.andi %68, %70 : i1
    %72 = acir.extract %67 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc32)
    %73 = acir.extract %72 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc32)
    %74 = arith.andi %71, %73 : i1
    %75 = acir.extract %67 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc33)
    %76 = acir.extract %75 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc33)
    %77 = arith.andi %74, %76 : i1
    %value_4, %ready_5 = acir.read(%66) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %78 = acir.bind %value_4 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    %79 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc35)
    %80 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %79, %79) : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    acir.path %54 {branch = 3 : i64, kind = "branch", txn = 0 : i64} loc(#loc27)
    acir.path %54 {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc27)
    acir.path %54 {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take", "top.left:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc27)
    %value_6, %ready_7 = acir.read(%54) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take", "top.left:issuable"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %81 = acir.bind %value_6 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
    %82 = acir.extract %81 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc30)
    %83 = acir.extract %81 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc31)
    %84 = arith.cmpi eq, %83, %c0_i32 : i32 loc(#loc31)
    %85 = arith.andi %82, %84 : i1
    %86 = acir.extract %81 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc32)
    %87 = acir.extract %86 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc32)
    %88 = arith.andi %85, %87 : i1
    %89 = acir.extract %81 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc33)
    %90 = acir.extract %89 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc33)
    %91 = arith.andi %88, %90 : i1
    %value_8, %ready_9 = acir.read(%54) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %92 = acir.bind %value_8 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc37)
    %93 = arith.select %54, %92, %78 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %94 = arith.select %54, %91, %77 : i1
    %95 = arith.andi %57, %94 : i1
    acir.path %95 {branch = 4 : i64, kind = "branch", txn = 0 : i64}
    %value_10, %ready_11 = acir.read(%95) {branch = 4 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.add_requests", txn = 0 : i64} : (i1) -> (i1, i1)
    %96 = arith.xori %value_10, %true : i1
    %97 = arith.andi %90, %96 : i1
    %98 = arith.andi %87, %97 : i1
    %99 = arith.andi %84, %98 : i1
    %100 = arith.andi %82, %99 : i1
    %101 = arith.andi %76, %96 : i1
    %102 = arith.andi %73, %101 : i1
    %103 = arith.andi %70, %102 : i1
    %104 = arith.andi %68, %103 : i1
    %105 = arith.select %50, %100, %104 : i1
    %106 = arith.select %37, %105, %100 : i1
    %107 = arith.select %25, %106, %100 : i1
    %108 = arith.select %19, %107, %100 : i1
    %109 = arith.select %17, %108, %100 : i1
    %110 = arith.select %31, %109, %104 : i1
    %111 = arith.select %12, %110, %104 : i1
    %112 = arith.select %6, %111, %104 : i1
    %113 = arith.select %4, %112, %104 : i1
    %114 = acir.fire "top.scheduler.issue_add#0" %113 {successors = [], txn = 0 : i64}
    %115 = arith.xori %6, %true : i1
    %116 = arith.xori %12, %true : i1
    %117 = arith.cmpi ne, %30, %c0_i32 : i32 loc(#loc12)
    %118 = arith.cmpi uge, %47, %48 : i32 loc(#loc21)
    %119 = arith.andi %37, %118 : i1
    %120 = arith.andi %25, %119 : i1
    %121 = arith.andi %19, %120 : i1
    %122 = arith.andi %17, %121 : i1
    %123 = arith.ori %117, %122 : i1
    %124 = arith.ori %116, %123 : i1
    %125 = arith.ori %115, %124 : i1
    %126 = arith.ori %8, %125 : i1
    %127 = arith.andi %114, %126 : i1
    acir.effect %127(%80) {branch = 2 : i64, frames = ["top.scheduler:issue_add", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc38)
    %128 = arith.xori %19, %true : i1
    %129 = arith.xori %25, %true : i1
    %130 = arith.cmpi ne, %36, %c0_i32 : i32 loc(#loc14)
    %131 = arith.ori %130, %50 : i1
    %132 = arith.ori %129, %131 : i1
    %133 = arith.ori %128, %132 : i1
    %134 = arith.ori %21, %133 : i1
    %135 = arith.andi %31, %134 : i1
    %136 = arith.andi %12, %135 : i1
    %137 = arith.andi %6, %136 : i1
    %138 = arith.andi %4, %137 : i1
    %139 = arith.andi %114, %138 : i1
    acir.effect %139(%80) {branch = 3 : i64, frames = ["top.scheduler:issue_add", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc38)
    %140 = arith.andi %87, %90 : i1
    %141 = arith.andi %84, %140 : i1
    %142 = arith.andi %82, %141 : i1
    %143 = arith.andi %73, %76 : i1
    %144 = arith.andi %70, %143 : i1
    %145 = arith.andi %68, %144 : i1
    %146 = arith.select %50, %142, %145 : i1
    %147 = arith.select %37, %146, %142 : i1
    %148 = arith.select %25, %147, %142 : i1
    %149 = arith.select %19, %148, %142 : i1
    %150 = arith.select %17, %149, %142 : i1
    %151 = arith.select %31, %150, %145 : i1
    %152 = arith.select %12, %151, %145 : i1
    %153 = arith.select %6, %152, %145 : i1
    %154 = arith.select %4, %153, %145 : i1
    %155 = arith.andi %114, %154 : i1
    acir.effect %155(%93) {branch = 4 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.add_requests", txn = 0 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
    acir.path %true {branch = 5 : i64, kind = "branch", txn = 1 : i64} loc(#loc39)
    acir.path %true {branch = 5 : i64, frames = ["top.scheduler:issue_mul"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc39)
    %156 = arith.xori %62, %true : i1 loc(#loc40)
    acir.path %156 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc41)
    acir.path %156 {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc41)
    acir.path %156 {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take", "top.right:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc28)
    %value_12, %ready_13 = acir.read(%156) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take", "top.right:issuable"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %157 = acir.bind %value_12 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc29)
    %158 = acir.extract %157 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc30)
    %159 = acir.extract %157 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc31)
    %160 = arith.cmpi eq, %159, %c1_i32 : i32 loc(#loc31)
    %161 = arith.andi %158, %160 : i1
    %162 = acir.extract %157 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc32)
    %163 = acir.extract %162 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc32)
    %164 = arith.andi %161, %163 : i1
    %165 = acir.extract %157 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc33)
    %166 = acir.extract %165 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc33)
    %167 = arith.andi %164, %166 : i1
    %value_14, %ready_15 = acir.read(%156) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %168 = acir.bind %value_14 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    acir.path %62 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc40)
    acir.path %62 {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc40)
    acir.path %62 {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take", "top.left:issuable"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc27)
    %value_16, %ready_17 = acir.read(%62) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take", "top.left:issuable"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %169 = acir.bind %value_16 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
    %170 = acir.extract %169 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc30)
    %171 = acir.extract %169 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc31)
    %172 = arith.cmpi eq, %171, %c1_i32 : i32 loc(#loc31)
    %173 = arith.andi %170, %172 : i1
    %174 = acir.extract %169 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc32)
    %175 = acir.extract %174 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc32)
    %176 = arith.andi %173, %175 : i1
    %177 = acir.extract %169 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc33)
    %178 = acir.extract %177 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc33)
    %179 = arith.andi %176, %178 : i1
    %value_18, %ready_19 = acir.read(%62) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %180 = acir.bind %value_18 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc37)
    %181 = arith.select %62, %180, %168 : !acir.struct<"src_payloads_25ebcaa7.Entry">
    %182 = arith.select %62, %179, %167 : i1
    %183 = arith.andi %65, %182 : i1
    acir.path %183 {branch = 8 : i64, kind = "branch", txn = 1 : i64}
    %value_20, %ready_21 = acir.read(%183) {branch = 8 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mul_requests", txn = 1 : i64} : (i1) -> (i1, i1)
    %184 = arith.xori %value_20, %true : i1
    %185 = arith.andi %178, %184 : i1
    %186 = arith.andi %175, %185 : i1
    %187 = arith.andi %172, %186 : i1
    %188 = arith.andi %170, %187 : i1
    %189 = arith.andi %166, %184 : i1
    %190 = arith.andi %163, %189 : i1
    %191 = arith.andi %160, %190 : i1
    %192 = arith.andi %158, %191 : i1
    %193 = arith.select %50, %188, %192 : i1
    %194 = arith.select %44, %193, %188 : i1
    %195 = arith.select %25, %194, %188 : i1
    %196 = arith.select %19, %195, %188 : i1
    %197 = arith.select %17, %196, %188 : i1
    %198 = arith.select %42, %197, %192 : i1
    %199 = arith.select %12, %198, %192 : i1
    %200 = arith.select %6, %199, %192 : i1
    %201 = arith.select %4, %200, %192 : i1
    %202 = acir.fire "top.scheduler.issue_mul#1" %201 {successors = [], txn = 1 : i64}
    %203 = arith.cmpi ne, %30, %c1_i32 : i32 loc(#loc16)
    %204 = arith.andi %44, %118 : i1
    %205 = arith.andi %25, %204 : i1
    %206 = arith.andi %19, %205 : i1
    %207 = arith.andi %17, %206 : i1
    %208 = arith.ori %203, %207 : i1
    %209 = arith.ori %116, %208 : i1
    %210 = arith.ori %115, %209 : i1
    %211 = arith.ori %8, %210 : i1
    %212 = arith.andi %202, %211 : i1
    acir.effect %212(%80) {branch = 6 : i64, frames = ["top.scheduler:issue_mul", "top.right:take"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc38)
    %213 = arith.cmpi ne, %36, %c1_i32 : i32 loc(#loc18)
    %214 = arith.ori %213, %50 : i1
    %215 = arith.ori %129, %214 : i1
    %216 = arith.ori %128, %215 : i1
    %217 = arith.ori %21, %216 : i1
    %218 = arith.andi %42, %217 : i1
    %219 = arith.andi %12, %218 : i1
    %220 = arith.andi %6, %219 : i1
    %221 = arith.andi %4, %220 : i1
    %222 = arith.andi %202, %221 : i1
    acir.effect %222(%80) {branch = 7 : i64, frames = ["top.scheduler:issue_mul", "top.left:take"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc38)
    %223 = arith.andi %175, %178 : i1
    %224 = arith.andi %172, %223 : i1
    %225 = arith.andi %170, %224 : i1
    %226 = arith.andi %163, %166 : i1
    %227 = arith.andi %160, %226 : i1
    %228 = arith.andi %158, %227 : i1
    %229 = arith.select %50, %225, %228 : i1
    %230 = arith.select %44, %229, %225 : i1
    %231 = arith.select %25, %230, %225 : i1
    %232 = arith.select %19, %231, %225 : i1
    %233 = arith.select %17, %232, %225 : i1
    %234 = arith.select %42, %233, %228 : i1
    %235 = arith.select %12, %234, %228 : i1
    %236 = arith.select %6, %235, %228 : i1
    %237 = arith.select %4, %236, %228 : i1
    %238 = arith.andi %202, %237 : i1
    acir.effect %238(%181) {branch = 8 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mul_requests", txn = 1 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
    acir.path %true {branch = 9 : i64, kind = "branch", txn = 2 : i64} loc(#loc42)
    acir.path %true {branch = 9 : i64, frames = ["top.adder:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc42)
    %value_22, %ready_23 = acir.read(%true) {branch = 9 : i64, frames = ["top.adder:execute"], method = "peek", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %239 = acir.bind %value_22 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc43)
    %240 = acir.extract %239 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc44)
    %241 = acir.extract %239 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc45)
    %242 = acir.extract %239 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc46)
    %243 = acir.extract %242 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc46)
    %244 = acir.extract %239 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc47)
    %245 = acir.extract %244 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc47)
    %246 = arith.addi %243, %245 : i32 loc(#loc46)
    %247 = acir.aggregate(%240, %241, %246) : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc48)
    acir.path %ready_23 {branch = 10 : i64, kind = "branch", txn = 2 : i64}
    %value_24, %ready_25 = acir.read(%ready_23) {branch = 10 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.add_done", txn = 2 : i64} : (i1) -> (i1, i1)
    %248 = arith.xori %value_24, %true : i1
    %249 = arith.andi %ready_23, %248 : i1
    %250 = acir.fire "top.adder.execute#2" %249 {successors = [0], txn = 2 : i64}
    acir.effect %250() {branch = 9 : i64, frames = ["top.adder:execute"], method = "pop", port = 0 : i64, resource = "top.add_requests", txn = 2 : i64} : (i1) -> ()
    %251 = arith.andi %250, %ready_23 : i1
    acir.effect %251(%247) {branch = 10 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.add_done", txn = 2 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> ()
    %value_26, %ready_27 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.multiplier.active", txn = -1 : i64} : (i1) -> (i1, i1)
    %252 = acir.bind %value_26 : (i1) -> i1 loc(#loc49)
    %253 = arith.xori %252, %true : i1
    acir.path %253 {branch = 11 : i64, kind = "branch", txn = 3 : i64} loc(#loc50)
    acir.path %253 {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc50)
    %value_28, %ready_29 = acir.read(%253) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "peek", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc51)
    %254 = arith.xori %value_26, %true : i1 loc(#loc52)
    %255 = arith.andi %254, %ready_29 : i1 loc(#loc52)
    %256 = acir.fire "top.multiplier.accept#3" %255 {successors = [1], txn = 3 : i64}
    %257 = arith.andi %256, %254 : i1
    acir.effect %257() {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "pop", port = 0 : i64, resource = "top.mul_requests", txn = 3 : i64} : (i1) -> () loc(#loc51)
    acir.effect %257(%value_28) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.request", txn = 3 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc52)
    acir.effect %257(%c2_i32) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 3 : i64} : (i1, i32) -> () loc(#loc53)
    acir.effect %257(%true) {branch = 11 : i64, frames = ["top.multiplier:accept"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 3 : i64} : (i1, i1) -> () loc(#loc54)
    acir.path %252 {branch = 12 : i64, kind = "branch", txn = 4 : i64} loc(#loc55)
    acir.path %252 {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc55)
    %258 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    %value_30, %ready_31 = acir.read(%252) {branch = 12 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc56)
    %259 = arith.cmpi ne, %value_30, %c0_i32 : i32 loc(#loc56)
    %260 = arith.cmpi eq, %value_30, %c0_i32 : i32 loc(#loc56)
    %261 = arith.andi %252, %259 : i1
    acir.path %261 {branch = 13 : i64, kind = "branch", txn = 4 : i64} loc(#loc57)
    %value_32, %ready_33 = acir.read(%261) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc57)
    %262 = arith.subi %value_32, %c1_i32 : i32 loc(#loc57)
    %263 = arith.andi %252, %260 : i1
    acir.path %263 {branch = 14 : i64, kind = "branch", txn = 4 : i64}
    %value_34, %ready_35 = acir.read(%263) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "read", port = 0 : i64, resource = "top.multiplier.request", txn = 4 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %264 = acir.bind %value_34 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc58)
    %265 = acir.extract %264 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc59)
    %266 = acir.extract %264 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc60)
    %267 = acir.extract %264 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc61)
    %268 = acir.extract %267 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc61)
    %269 = acir.extract %264 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc62)
    %270 = acir.extract %269 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc62)
    %271 = arith.muli %268, %270 : i32 loc(#loc61)
    %272 = acir.aggregate(%265, %266, %271) : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc63)
    %273 = arith.select %260, %272, %258 : !acir.struct<"src_payloads_25ebcaa7.Completion">
    acir.path %263 {branch = 15 : i64, kind = "branch", txn = 4 : i64}
    %value_36, %ready_37 = acir.read(%263) {branch = 15 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mul_done", txn = 4 : i64} : (i1) -> (i1, i1)
    %274 = arith.xori %value_36, %true : i1
    %275 = arith.ori %259, %274 : i1
    %276 = arith.andi %value_26, %275 : i1
    %277 = acir.fire "top.multiplier.advance#4" %276 {successors = [], txn = 4 : i64}
    %278 = arith.andi %value_26, %259 : i1
    %279 = arith.andi %277, %278 : i1
    acir.effect %279(%262) {branch = 13 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.remaining", txn = 4 : i64} : (i1, i32) -> () loc(#loc64)
    %280 = arith.andi %value_26, %260 : i1
    %281 = arith.andi %277, %280 : i1
    acir.effect %281(%false) {branch = 14 : i64, frames = ["top.multiplier:advance"], method = "write", port = 0 : i64, resource = "top.multiplier.active", txn = 4 : i64} : (i1, i1) -> () loc(#loc65)
    acir.effect %281(%273) {branch = 15 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mul_done", txn = 4 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> ()
    %value_38, %ready_39 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.add_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc66)
    %282 = arith.xori %value_38, %true : i1 loc(#loc67)
    acir.path %true {branch = 16 : i64, kind = "branch", txn = 5 : i64} loc(#loc68)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc68)
    acir.path %value_38 {branch = 17 : i64, kind = "branch", txn = 5 : i64}
    %value_40, %ready_41 = acir.read(%value_38) {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1)
    %283 = acir.bind %value_40 : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc69)
    acir.path %282 {branch = 18 : i64, kind = "branch", txn = 5 : i64}
    %value_42, %ready_43 = acir.read(%282) {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "peek", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1)
    %284 = acir.bind %value_42 : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc69)
    %285 = arith.select %value_38, %283, %284 : !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc208)
    %286 = arith.select %value_38, %ready_41, %ready_43 : i1 loc(#loc67)
    %287 = acir.extract %285 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc70)
    %288 = acir.extract %285 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc71)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.rob:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc72)
    %289 = arith.subi %287, %c1_i32 : i32 loc(#loc73)
    %290 = arith.andi %289, %c3_i32 : i32 loc(#loc74)
    %291 = arith.cmpi eq, %290, %c0_i32 : i32 loc(#loc75)
    %292 = arith.cmpi eq, %290, %c1_i32 : i32 loc(#loc76)
    %293 = arith.cmpi eq, %290, %c2_i32 : i32 loc(#loc77)
    %294 = arith.cmpi eq, %290, %c3_i32 : i32 loc(#loc78)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc79)
    %295 = arith.cmpi ne, %290, %c0_i32 : i32 loc(#loc75)
    %296 = arith.cmpi ne, %290, %c1_i32 : i32 loc(#loc76)
    %297 = arith.andi %295, %296 : i1
    %298 = arith.cmpi ne, %290, %c2_i32 : i32 loc(#loc77)
    %299 = arith.andi %297, %298 : i1
    %300 = arith.andi %299, %294 : i1
    acir.path %300 {branch = 19 : i64, kind = "branch", txn = 5 : i64} loc(#loc79)
    acir.path %300 {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.3:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc79)
    %value_44, %ready_45 = acir.read(%300) {branch = 19 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.3:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %301 = acir.bind %value_44 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc80)
    %302 = acir.extract %301 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc81)
    %303 = acir.extract %301 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc82)
    %304 = arith.xori %303, %true : i1 loc(#loc83)
    %305 = arith.andi %302, %304 : i1
    %306 = acir.extract %301 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc84)
    %307 = arith.cmpi eq, %306, %287 : i32 loc(#loc84)
    %308 = arith.andi %305, %307 : i1
    %309 = arith.andi %300, %308 : i1
    %310 = acir.extract %301 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %311 = acir.extract %301 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %312 = acir.aggregate(%302, %true, %306, %310, %311) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc80)
    %313 = acir.extract %312 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %314 = acir.extract %312 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %315 = acir.extract %312 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %316 = acir.extract %312 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %317 = acir.aggregate(%313, %314, %315, %316, %288) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc80)
    acir.path %309 {branch = 20 : i64, kind = "branch", txn = 5 : i64} loc(#loc87)
    %318 = arith.andi %297, %293 : i1
    acir.path %318 {branch = 21 : i64, kind = "branch", txn = 5 : i64} loc(#loc79)
    acir.path %318 {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.2:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc79)
    %value_46, %ready_47 = acir.read(%318) {branch = 21 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.2:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %319 = acir.bind %value_46 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
    %320 = acir.extract %319 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc81)
    %321 = acir.extract %319 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc82)
    %322 = arith.xori %321, %true : i1 loc(#loc83)
    %323 = arith.andi %320, %322 : i1
    %324 = acir.extract %319 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc84)
    %325 = arith.cmpi eq, %324, %287 : i32 loc(#loc84)
    %326 = arith.andi %323, %325 : i1
    %327 = arith.andi %318, %326 : i1
    %328 = acir.extract %319 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %329 = acir.extract %319 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %330 = acir.aggregate(%320, %true, %324, %328, %329) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
    %331 = acir.extract %330 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %332 = acir.extract %330 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %333 = acir.extract %330 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %334 = acir.extract %330 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %335 = acir.aggregate(%331, %332, %333, %334, %288) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
    acir.path %327 {branch = 22 : i64, kind = "branch", txn = 5 : i64} loc(#loc87)
    %336 = arith.andi %295, %292 : i1
    acir.path %336 {branch = 23 : i64, kind = "branch", txn = 5 : i64} loc(#loc79)
    acir.path %336 {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.1:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc79)
    %value_48, %ready_49 = acir.read(%336) {branch = 23 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.1:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %337 = acir.bind %value_48 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
    %338 = acir.extract %337 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc81)
    %339 = acir.extract %337 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc82)
    %340 = arith.xori %339, %true : i1 loc(#loc83)
    %341 = arith.andi %338, %340 : i1
    %342 = acir.extract %337 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc84)
    %343 = arith.cmpi eq, %342, %287 : i32 loc(#loc84)
    %344 = arith.andi %341, %343 : i1
    %345 = arith.andi %336, %344 : i1
    %346 = acir.extract %337 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %347 = acir.extract %337 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %348 = acir.aggregate(%338, %true, %342, %346, %347) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
    %349 = acir.extract %348 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %350 = acir.extract %348 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %351 = acir.extract %348 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %352 = acir.extract %348 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %353 = acir.aggregate(%349, %350, %351, %352, %288) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
    acir.path %345 {branch = 24 : i64, kind = "branch", txn = 5 : i64} loc(#loc87)
    acir.path %291 {branch = 25 : i64, kind = "branch", txn = 5 : i64} loc(#loc79)
    acir.path %291 {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.0:complete"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc79)
    %value_50, %ready_51 = acir.read(%291) {branch = 25 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.0:complete"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %354 = acir.bind %value_50 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc90)
    %355 = acir.extract %354 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc81)
    %356 = acir.extract %354 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc82)
    %357 = arith.xori %356, %true : i1 loc(#loc83)
    %358 = arith.andi %355, %357 : i1
    %359 = acir.extract %354 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc84)
    %360 = arith.cmpi eq, %359, %287 : i32 loc(#loc84)
    %361 = arith.andi %358, %360 : i1
    %362 = arith.andi %291, %361 : i1
    %363 = acir.extract %354 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %364 = acir.extract %354 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc85)
    %365 = acir.aggregate(%355, %true, %359, %363, %364) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc90)
    %366 = acir.extract %365 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %367 = acir.extract %365 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc86)
    %368 = acir.extract %365 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %369 = acir.extract %365 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc86)
    %370 = acir.aggregate(%366, %367, %368, %369, %288) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc90)
    acir.path %362 {branch = 26 : i64, kind = "branch", txn = 5 : i64} loc(#loc87)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc91)
    %value_52, %ready_53 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %371 = acir.bind %value_52 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %372 = acir.extract %371 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc93)
    %373 = acir.extract %371 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc94)
    %374 = acir.extract %373 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc94)
    %375 = acir.extract %373 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc95)
    %376 = arith.cmpi eq, %375, %287 : i32 loc(#loc95)
    %377 = arith.xori %374, %true : i1
    %378 = arith.andi %377, %376 : i1 loc(#loc96)
    %379 = acir.aggregate(%true, %287, %288) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc97)
    %380 = acir.extract %371 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc98)
    %381 = acir.extract %371 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc98)
    %382 = acir.extract %371 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc98)
    %383 = acir.extract %371 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc98)
    %384 = acir.aggregate(%372, %380, %381, %382, %379, %383) : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %385 = arith.select %378, %384, %371 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %386 = acir.extract %385 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc99)
    %387 = acir.extract %386 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc99)
    %388 = acir.extract %386 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc100)
    %389 = arith.cmpi eq, %388, %287 : i32 loc(#loc100)
    %390 = arith.xori %387, %true : i1
    %391 = arith.andi %390, %389 : i1
    %392 = arith.select %391, %true, %378 : i1 loc(#loc96)
    %393 = acir.extract %385 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc101)
    %394 = acir.extract %385 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc101)
    %395 = acir.extract %385 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc101)
    %396 = acir.extract %385 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc101)
    %397 = acir.extract %385 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc101)
    %398 = acir.aggregate(%393, %394, %395, %396, %397, %379) : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %399 = arith.select %391, %398, %385 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    %400 = arith.andi %372, %392 : i1 loc(#loc96)
    %401 = arith.select %372, %399, %371 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc92)
    acir.path %400 {branch = 27 : i64, kind = "branch", txn = 5 : i64} loc(#loc102)
    acir.path %true {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc103)
    %value_54, %ready_55 = acir.read(%true) {branch = 16 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1)
    %402 = acir.bind %value_54 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc104)
    %403 = acir.extract %402 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc93)
    %404 = acir.extract %402 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc94)
    %405 = acir.extract %404 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc94)
    %406 = acir.extract %404 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc95)
    %407 = arith.cmpi eq, %406, %287 : i32 loc(#loc95)
    %408 = arith.xori %405, %true : i1
    %409 = arith.andi %408, %407 : i1 loc(#loc105)
    %410 = acir.extract %402 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc98)
    %411 = acir.extract %402 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc98)
    %412 = acir.extract %402 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc98)
    %413 = acir.extract %402 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc98)
    %414 = acir.aggregate(%403, %410, %411, %412, %379, %413) : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc104)
    %415 = arith.select %409, %414, %402 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc104)
    %416 = acir.extract %415 {field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc99)
    %417 = acir.extract %416 {field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc99)
    %418 = acir.extract %416 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc100)
    %419 = arith.cmpi eq, %418, %287 : i32 loc(#loc100)
    %420 = arith.xori %417, %true : i1
    %421 = arith.andi %420, %419 : i1
    %422 = arith.select %421, %true, %409 : i1 loc(#loc105)
    %423 = acir.extract %415 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc101)
    %424 = acir.extract %415 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc101)
    %425 = acir.extract %415 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc101)
    %426 = acir.extract %415 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc101)
    %427 = acir.extract %415 {field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc101)
    %428 = acir.aggregate(%423, %424, %425, %426, %427, %379) : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc104)
    %429 = arith.select %421, %428, %415 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc104)
    %430 = arith.andi %403, %422 : i1 loc(#loc105)
    %431 = arith.select %403, %429, %402 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc104)
    acir.path %430 {branch = 28 : i64, kind = "branch", txn = 5 : i64} loc(#loc102)
    acir.path %286 {branch = 29 : i64, kind = "branch", txn = 5 : i64}
    %432 = arith.ori %293, %294 : i1 loc(#loc102)
    %433 = arith.ori %292, %432 : i1 loc(#loc102)
    %434 = arith.ori %291, %433 : i1 loc(#loc102)
    %435 = arith.andi %ready_41, %434 : i1 loc(#loc102)
    %436 = arith.select %value_38, %435, %434 : i1 loc(#loc102)
    %437 = acir.fire "top.cdb.broadcast#5" %436 {successors = [2, 4, 8], txn = 5 : i64}
    %438 = arith.andi %437, %value_38 : i1
    acir.effect %438() {branch = 17 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.mul_done", txn = 5 : i64} : (i1) -> ()
    %439 = arith.andi %437, %282 : i1
    acir.effect %439() {branch = 18 : i64, frames = ["top.cdb:broadcast"], method = "pop", port = 0 : i64, resource = "top.add_done", txn = 5 : i64} : (i1) -> ()
    %440 = arith.andi %304, %307 : i1
    %441 = arith.andi %302, %440 : i1
    %442 = arith.andi %294, %441 : i1
    %443 = arith.andi %298, %442 : i1
    %444 = arith.andi %296, %443 : i1
    %445 = arith.andi %295, %444 : i1
    %446 = arith.andi %437, %445 : i1
    acir.effect %446(%317) {branch = 20 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.3:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc87)
    %447 = arith.andi %322, %325 : i1
    %448 = arith.andi %320, %447 : i1
    %449 = arith.andi %293, %448 : i1
    %450 = arith.andi %296, %449 : i1
    %451 = arith.andi %295, %450 : i1
    %452 = arith.andi %437, %451 : i1
    acir.effect %452(%335) {branch = 22 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.2:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc87)
    %453 = arith.andi %340, %343 : i1
    %454 = arith.andi %338, %453 : i1
    %455 = arith.andi %292, %454 : i1
    %456 = arith.andi %295, %455 : i1
    %457 = arith.andi %437, %456 : i1
    acir.effect %457(%353) {branch = 24 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.1:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc87)
    %458 = arith.andi %357, %360 : i1
    %459 = arith.andi %355, %458 : i1
    %460 = arith.andi %291, %459 : i1
    %461 = arith.andi %437, %460 : i1
    acir.effect %461(%370) {branch = 26 : i64, frames = ["top.cdb:broadcast", "top.rob:complete", "top.rob:$dispatch11", "top.rob.slots.0:complete"], method = "write", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc87)
    %462 = arith.ori %389, %378 : i1
    %463 = arith.select %387, %378, %462 : i1
    %464 = arith.andi %372, %463 : i1
    %465 = arith.andi %437, %464 : i1
    acir.effect %465(%401) {branch = 27 : i64, frames = ["top.cdb:broadcast", "top.left:wakeup"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc102)
    %466 = arith.ori %419, %409 : i1
    %467 = arith.select %417, %409, %466 : i1
    %468 = arith.andi %403, %467 : i1
    %469 = arith.andi %437, %468 : i1
    acir.effect %469(%431) {branch = 28 : i64, frames = ["top.cdb:broadcast", "top.right:wakeup"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc102)
    %470 = arith.ori %282, %ready_41 : i1
    %471 = arith.andi %437, %470 : i1
    acir.effect %471(%285) {branch = 29 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.event", txn = 5 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) -> ()
    %value_56, %ready_57 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.add_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc106)
    acir.path %value_56 {branch = 30 : i64, kind = "branch", txn = -1 : i64} loc(#loc107)
    %value_58, %ready_59 = acir.read(%value_56) {branch = 30 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.mul_done", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc107)
    %472 = arith.andi %value_56, %value_58 : i1
    acir.path %472 {branch = 31 : i64, kind = "branch", txn = 6 : i64} loc(#loc108)
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc108)
    %value_60, %ready_61 = acir.read(%472) {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "peek", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, i1)
    %473 = acir.bind %value_60 : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc109)
    %474 = acir.extract %473 {field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc110)
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64}
    %475 = arith.cmpi ult, %474, %c8_i32 : i32 loc(#loc111)
    %476 = arith.cmpi eq, %474, %c0_i32 : i32 loc(#loc112)
    %477 = arith.cmpi ne, %474, %c0_i32 : i32 loc(#loc112)
    %478 = acir.aggregate(%true, %c0_i32, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc113)
    %479 = arith.select %476, %478, %79 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    "acir.resource.array"() <{name = "top.rf.renamed", resources = ["top.rf.renamed.0", "top.rf.renamed.1", "top.rf.renamed.2", "top.rf.renamed.3", "top.rf.renamed.4", "top.rf.renamed.5", "top.rf.renamed.6", "top.rf.renamed.7"], shape = [8]}> : () -> () loc(#loc114)
    "acir.resource.array"() <{name = "top.rf.committed", resources = ["top.rf.committed.0", "top.rf.committed.1", "top.rf.committed.2", "top.rf.committed.3", "top.rf.committed.4", "top.rf.committed.5", "top.rf.committed.6", "top.rf.committed.7"], shape = [8]}> : () -> () loc(#loc115)
    "acir.resource.array"() <{name = "top.rf.registers", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7"], shape = [8]}> : () -> () loc(#loc116)
    %480 = arith.andi %472, %477 : i1
    %481 = arith.extui %474 : i32 to i64 loc(#loc117)
    %482 = arith.cmpi ult, %481, %c8_i64 : i64 loc(#loc117)
    acir.path %480 {branch = 32 : i64, kind = "branch", txn = 6 : i64}
    %value_62, %ready_63 = acir.read(%480, %481) {array = "top.rf.renamed", branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.renamed.0", resources = ["top.rf.renamed.0", "top.rf.renamed.1", "top.rf.renamed.2", "top.rf.renamed.3", "top.rf.renamed.4", "top.rf.renamed.5", "top.rf.renamed.6", "top.rf.renamed.7"], txn = 6 : i64} : (i1, i64) -> (i32, i1)
    %483 = arith.cmpi eq, %481, %c0_i64 : i64 loc(#loc118)
    %484 = arith.andi %480, %483 : i1 loc(#loc118)
    acir.path %484 {branch = 33 : i64, kind = "branch", txn = 6 : i64}
    %485 = arith.cmpi eq, %481, %c1_i64 : i64 loc(#loc118)
    %486 = arith.andi %480, %485 : i1 loc(#loc118)
    acir.path %486 {branch = 34 : i64, kind = "branch", txn = 6 : i64}
    %487 = arith.cmpi eq, %481, %c2_i64 : i64 loc(#loc118)
    %488 = arith.andi %480, %487 : i1 loc(#loc118)
    acir.path %488 {branch = 35 : i64, kind = "branch", txn = 6 : i64}
    %489 = arith.cmpi eq, %481, %c3_i64 : i64 loc(#loc118)
    %490 = arith.andi %480, %489 : i1 loc(#loc118)
    acir.path %490 {branch = 36 : i64, kind = "branch", txn = 6 : i64}
    %491 = arith.cmpi eq, %481, %c4_i64 : i64 loc(#loc118)
    %492 = arith.andi %480, %491 : i1 loc(#loc118)
    acir.path %492 {branch = 37 : i64, kind = "branch", txn = 6 : i64}
    %493 = arith.cmpi eq, %481, %c5_i64 : i64 loc(#loc118)
    %494 = arith.andi %480, %493 : i1 loc(#loc118)
    acir.path %494 {branch = 38 : i64, kind = "branch", txn = 6 : i64}
    %495 = arith.cmpi eq, %481, %c6_i64 : i64 loc(#loc118)
    %496 = arith.andi %480, %495 : i1 loc(#loc118)
    acir.path %496 {branch = 39 : i64, kind = "branch", txn = 6 : i64}
    %497 = arith.cmpi eq, %481, %c7_i64 : i64 loc(#loc118)
    %498 = arith.andi %480, %497 : i1 loc(#loc118)
    acir.path %498 {branch = 40 : i64, kind = "branch", txn = 6 : i64}
    %499 = acir.bind %value_62 : (i32) -> i32 loc(#loc119)
    %value_64, %ready_65 = acir.read(%480, %481) {array = "top.rf.committed", branch = 32 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.committed.0", resources = ["top.rf.committed.0", "top.rf.committed.1", "top.rf.committed.2", "top.rf.committed.3", "top.rf.committed.4", "top.rf.committed.5", "top.rf.committed.6", "top.rf.committed.7"], txn = 6 : i64} : (i1, i64) -> (i32, i1) loc(#loc118)
    acir.path %484 {branch = 41 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %486 {branch = 42 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %488 {branch = 43 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %490 {branch = 44 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %492 {branch = 45 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %494 {branch = 46 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %496 {branch = 47 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %498 {branch = 48 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    %500 = arith.cmpi eq, %499, %value_64 : i32 loc(#loc120)
    %501 = arith.cmpi ne, %499, %value_64 : i32 loc(#loc120)
    %502 = arith.andi %480, %500 : i1
    acir.path %502 {branch = 49 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %value_66, %ready_67 = acir.read(%502, %481) {array = "top.rf.registers", branch = 49 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.registers.0", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7"], txn = 6 : i64} : (i1, i64) -> (i32, i1) loc(#loc121)
    %503 = arith.andi %502, %483 : i1 loc(#loc121)
    acir.path %503 {branch = 50 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %504 = arith.andi %502, %485 : i1 loc(#loc121)
    acir.path %504 {branch = 51 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %505 = arith.andi %502, %487 : i1 loc(#loc121)
    acir.path %505 {branch = 52 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %506 = arith.andi %502, %489 : i1 loc(#loc121)
    acir.path %506 {branch = 53 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %507 = arith.andi %502, %491 : i1 loc(#loc121)
    acir.path %507 {branch = 54 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %508 = arith.andi %502, %493 : i1 loc(#loc121)
    acir.path %508 {branch = 55 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %509 = arith.andi %502, %495 : i1 loc(#loc121)
    acir.path %509 {branch = 56 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %510 = arith.andi %502, %497 : i1 loc(#loc121)
    acir.path %510 {branch = 57 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %511 = acir.aggregate(%true, %499, %value_66) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc122)
    %512 = arith.select %500, %511, %479 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %513 = arith.andi %480, %501 : i1
    acir.path %513 {branch = 58 : i64, kind = "branch", txn = 6 : i64}
    acir.path %513 {branch = 58 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64}
    %514 = arith.subi %499, %c1_i32 : i32 loc(#loc123)
    %515 = arith.andi %514, %c3_i32 : i32 loc(#loc124)
    %516 = arith.cmpi eq, %515, %c0_i32 : i32 loc(#loc125)
    %517 = arith.cmpi eq, %515, %c1_i32 : i32 loc(#loc126)
    %518 = arith.cmpi eq, %515, %c2_i32 : i32 loc(#loc127)
    %519 = arith.cmpi eq, %515, %c3_i32 : i32 loc(#loc128)
    acir.path %513 {branch = 58 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %520 = acir.aggregate() : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %521 = arith.cmpi ne, %515, %c0_i32 : i32 loc(#loc125)
    %522 = arith.andi %513, %521 : i1
    %523 = arith.cmpi ne, %515, %c1_i32 : i32 loc(#loc126)
    %524 = arith.andi %522, %523 : i1
    %525 = arith.cmpi ne, %515, %c2_i32 : i32 loc(#loc127)
    %526 = arith.andi %524, %525 : i1
    %527 = arith.cmpi ne, %515, %c3_i32 : i32 loc(#loc128)
    %528 = arith.andi %526, %519 : i1
    acir.path %528 {branch = 59 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %528 {branch = 59 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_68, %ready_69 = acir.read(%528) {branch = 59 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %529 = arith.select %519, %value_68, %520 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %530 = arith.andi %524, %518 : i1
    acir.path %530 {branch = 60 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %530 {branch = 60 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_70, %ready_71 = acir.read(%530) {branch = 60 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %531 = arith.andi %525, %527 : i1
    %532 = arith.select %518, %value_70, %529 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %533 = arith.andi %522, %517 : i1
    acir.path %533 {branch = 61 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %533 {branch = 61 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_72, %ready_73 = acir.read(%533) {branch = 61 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %534 = arith.andi %523, %531 : i1
    %535 = arith.select %517, %value_72, %532 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %536 = arith.andi %513, %516 : i1
    acir.path %536 {branch = 62 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %536 {branch = 62 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_74, %ready_75 = acir.read(%536) {branch = 62 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %537 = arith.andi %521, %534 : i1
    %538 = arith.select %516, %value_74, %535 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc131)
    %539 = arith.xori %537, %true : i1
    %540 = acir.extract %538 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc132)
    %541 = acir.extract %538 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
    %542 = arith.andi %540, %541 : i1
    %543 = acir.extract %538 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc134)
    %544 = arith.cmpi eq, %543, %499 : i32 loc(#loc134)
    %545 = arith.andi %542, %544 : i1
    %546 = arith.xori %545, %true : i1
    %547 = arith.select %545, %546, %false : i1
    %548 = acir.aggregate(%false, %499, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc135)
    %549 = acir.extract %538 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc136)
    %550 = acir.aggregate(%true, %499, %549) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc137)
    %551 = arith.select %545, %550, %548 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %552 = arith.andi %501, %547 : i1
    %553 = arith.select %501, %551, %512 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %554 = arith.andi %477, %552 : i1
    %555 = arith.select %477, %553, %479 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc138)
    %556 = arith.xori %554, %true : i1
    %557 = acir.extract %473 {field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc110)
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64}
    %558 = arith.cmpi ult, %557, %c8_i32 : i32 loc(#loc111)
    %559 = arith.cmpi eq, %557, %c0_i32 : i32 loc(#loc112)
    %560 = arith.cmpi ne, %557, %c0_i32 : i32 loc(#loc112)
    %561 = arith.select %559, %478, %79 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    "acir.resource.array"() <{name = "top.rf.renamed", resources = ["top.rf.renamed.0", "top.rf.renamed.1", "top.rf.renamed.2", "top.rf.renamed.3", "top.rf.renamed.4", "top.rf.renamed.5", "top.rf.renamed.6", "top.rf.renamed.7"], shape = [8]}> : () -> () loc(#loc114)
    "acir.resource.array"() <{name = "top.rf.committed", resources = ["top.rf.committed.0", "top.rf.committed.1", "top.rf.committed.2", "top.rf.committed.3", "top.rf.committed.4", "top.rf.committed.5", "top.rf.committed.6", "top.rf.committed.7"], shape = [8]}> : () -> () loc(#loc115)
    "acir.resource.array"() <{name = "top.rf.registers", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7"], shape = [8]}> : () -> () loc(#loc116)
    %562 = arith.andi %472, %560 : i1
    %563 = arith.extui %557 : i32 to i64 loc(#loc117)
    %564 = arith.cmpi ult, %563, %c8_i64 : i64 loc(#loc117)
    acir.path %562 {branch = 63 : i64, kind = "branch", txn = 6 : i64}
    %value_76, %ready_77 = acir.read(%562, %563) {array = "top.rf.renamed", branch = 63 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.renamed.0", resources = ["top.rf.renamed.0", "top.rf.renamed.1", "top.rf.renamed.2", "top.rf.renamed.3", "top.rf.renamed.4", "top.rf.renamed.5", "top.rf.renamed.6", "top.rf.renamed.7"], txn = 6 : i64} : (i1, i64) -> (i32, i1)
    %565 = arith.cmpi eq, %563, %c0_i64 : i64 loc(#loc118)
    %566 = arith.andi %562, %565 : i1 loc(#loc118)
    acir.path %566 {branch = 64 : i64, kind = "branch", txn = 6 : i64}
    %567 = arith.cmpi eq, %563, %c1_i64 : i64 loc(#loc118)
    %568 = arith.andi %562, %567 : i1 loc(#loc118)
    acir.path %568 {branch = 65 : i64, kind = "branch", txn = 6 : i64}
    %569 = arith.cmpi eq, %563, %c2_i64 : i64 loc(#loc118)
    %570 = arith.andi %562, %569 : i1 loc(#loc118)
    acir.path %570 {branch = 66 : i64, kind = "branch", txn = 6 : i64}
    %571 = arith.cmpi eq, %563, %c3_i64 : i64 loc(#loc118)
    %572 = arith.andi %562, %571 : i1 loc(#loc118)
    acir.path %572 {branch = 67 : i64, kind = "branch", txn = 6 : i64}
    %573 = arith.cmpi eq, %563, %c4_i64 : i64 loc(#loc118)
    %574 = arith.andi %562, %573 : i1 loc(#loc118)
    acir.path %574 {branch = 68 : i64, kind = "branch", txn = 6 : i64}
    %575 = arith.cmpi eq, %563, %c5_i64 : i64 loc(#loc118)
    %576 = arith.andi %562, %575 : i1 loc(#loc118)
    acir.path %576 {branch = 69 : i64, kind = "branch", txn = 6 : i64}
    %577 = arith.cmpi eq, %563, %c6_i64 : i64 loc(#loc118)
    %578 = arith.andi %562, %577 : i1 loc(#loc118)
    acir.path %578 {branch = 70 : i64, kind = "branch", txn = 6 : i64}
    %579 = arith.cmpi eq, %563, %c7_i64 : i64 loc(#loc118)
    %580 = arith.andi %562, %579 : i1 loc(#loc118)
    acir.path %580 {branch = 71 : i64, kind = "branch", txn = 6 : i64}
    %581 = acir.bind %value_76 : (i32) -> i32 loc(#loc119)
    %value_78, %ready_79 = acir.read(%562, %563) {array = "top.rf.committed", branch = 63 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.committed.0", resources = ["top.rf.committed.0", "top.rf.committed.1", "top.rf.committed.2", "top.rf.committed.3", "top.rf.committed.4", "top.rf.committed.5", "top.rf.committed.6", "top.rf.committed.7"], txn = 6 : i64} : (i1, i64) -> (i32, i1) loc(#loc118)
    acir.path %566 {branch = 72 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %568 {branch = 73 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %570 {branch = 74 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %572 {branch = 75 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %574 {branch = 76 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %576 {branch = 77 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %578 {branch = 78 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    acir.path %580 {branch = 79 : i64, kind = "branch", txn = 6 : i64} loc(#loc118)
    %582 = arith.cmpi eq, %581, %value_78 : i32 loc(#loc120)
    %583 = arith.cmpi ne, %581, %value_78 : i32 loc(#loc120)
    %584 = arith.andi %562, %582 : i1
    acir.path %584 {branch = 80 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %value_80, %ready_81 = acir.read(%584, %563) {array = "top.rf.registers", branch = 80 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.registers.0", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7"], txn = 6 : i64} : (i1, i64) -> (i32, i1) loc(#loc121)
    %585 = arith.andi %584, %565 : i1 loc(#loc121)
    acir.path %585 {branch = 81 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %586 = arith.andi %584, %567 : i1 loc(#loc121)
    acir.path %586 {branch = 82 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %587 = arith.andi %584, %569 : i1 loc(#loc121)
    acir.path %587 {branch = 83 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %588 = arith.andi %584, %571 : i1 loc(#loc121)
    acir.path %588 {branch = 84 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %589 = arith.andi %584, %573 : i1 loc(#loc121)
    acir.path %589 {branch = 85 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %590 = arith.andi %584, %575 : i1 loc(#loc121)
    acir.path %590 {branch = 86 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %591 = arith.andi %584, %577 : i1 loc(#loc121)
    acir.path %591 {branch = 87 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %592 = arith.andi %584, %579 : i1 loc(#loc121)
    acir.path %592 {branch = 88 : i64, kind = "branch", txn = 6 : i64} loc(#loc121)
    %593 = acir.aggregate(%true, %581, %value_80) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc122)
    %594 = arith.select %582, %593, %561 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %595 = arith.andi %562, %583 : i1
    acir.path %595 {branch = 89 : i64, kind = "branch", txn = 6 : i64}
    acir.path %595 {branch = 89 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64}
    %596 = arith.subi %581, %c1_i32 : i32 loc(#loc123)
    %597 = arith.andi %596, %c3_i32 : i32 loc(#loc124)
    %598 = arith.cmpi eq, %597, %c0_i32 : i32 loc(#loc125)
    %599 = arith.cmpi eq, %597, %c1_i32 : i32 loc(#loc126)
    %600 = arith.cmpi eq, %597, %c2_i32 : i32 loc(#loc127)
    %601 = arith.cmpi eq, %597, %c3_i32 : i32 loc(#loc128)
    acir.path %595 {branch = 89 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %602 = arith.cmpi ne, %597, %c0_i32 : i32 loc(#loc125)
    %603 = arith.andi %595, %602 : i1
    %604 = arith.cmpi ne, %597, %c1_i32 : i32 loc(#loc126)
    %605 = arith.andi %603, %604 : i1
    %606 = arith.cmpi ne, %597, %c2_i32 : i32 loc(#loc127)
    %607 = arith.andi %605, %606 : i1
    %608 = arith.cmpi ne, %597, %c3_i32 : i32 loc(#loc128)
    %609 = arith.andi %607, %601 : i1
    acir.path %609 {branch = 90 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %609 {branch = 90 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_82, %ready_83 = acir.read(%609) {branch = 90 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.3:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %610 = arith.select %601, %value_82, %520 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %611 = arith.andi %605, %600 : i1
    acir.path %611 {branch = 91 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %611 {branch = 91 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_84, %ready_85 = acir.read(%611) {branch = 91 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.2:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %612 = arith.andi %606, %608 : i1
    %613 = arith.select %600, %value_84, %610 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %614 = arith.andi %603, %599 : i1
    acir.path %614 {branch = 92 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %614 {branch = 92 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_86, %ready_87 = acir.read(%614) {branch = 92 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.1:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %615 = arith.andi %604, %612 : i1
    %616 = arith.select %599, %value_86, %613 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %617 = arith.andi %595, %598 : i1
    acir.path %617 {branch = 93 : i64, kind = "branch", txn = 6 : i64} loc(#loc129)
    acir.path %617 {branch = 93 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc129)
    %value_88, %ready_89 = acir.read(%617) {branch = 93 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20", "top.rob.slots.0:peek"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc130)
    %618 = arith.andi %602, %615 : i1
    %619 = arith.select %598, %value_88, %616 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc131)
    %620 = arith.xori %618, %true : i1
    %621 = acir.extract %619 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc132)
    %622 = acir.extract %619 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
    %623 = arith.andi %621, %622 : i1
    %624 = acir.extract %619 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc134)
    %625 = arith.cmpi eq, %624, %581 : i32 loc(#loc134)
    %626 = arith.andi %623, %625 : i1
    %627 = arith.xori %626, %true : i1
    %628 = arith.select %626, %627, %false : i1
    %629 = acir.aggregate(%false, %581, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc135)
    %630 = acir.extract %619 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc136)
    %631 = acir.aggregate(%true, %581, %630) : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc137)
    %632 = arith.select %626, %631, %629 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %633 = arith.andi %583, %628 : i1
    %634 = arith.select %583, %632, %594 : !acir.struct<"src_payloads_25ebcaa7.Operand">
    %635 = arith.andi %560, %633 : i1
    %636 = arith.select %560, %634, %561 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc139)
    %637 = arith.xori %635, %true : i1
    %638 = acir.extract %473 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc140)
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64}
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc141)
    %value_90, %ready_91 = acir.read(%472) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc142)
    %value_92, %ready_93 = acir.read(%472) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:room"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc143)
    %639 = arith.subi %value_90, %value_92 : i32 loc(#loc142)
    %640 = arith.cmpi ult, %639, %c4_i32 : i32 loc(#loc142)
    %value_94, %ready_95 = acir.read(%472) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "read", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1) -> (i32, i1)
    %641 = acir.bind %value_94 : (i32) -> i32 loc(#loc144)
    %642 = arith.subi %641, %c1_i32 : i32 loc(#loc145)
    %643 = arith.andi %642, %c3_i32 : i32 loc(#loc146)
    %644 = arith.cmpi eq, %643, %c0_i32 : i32 loc(#loc147)
    %645 = arith.cmpi eq, %643, %c1_i32 : i32 loc(#loc148)
    %646 = arith.cmpi eq, %643, %c2_i32 : i32 loc(#loc149)
    %647 = arith.cmpi eq, %643, %c3_i32 : i32 loc(#loc150)
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %648 = arith.cmpi ne, %643, %c0_i32 : i32 loc(#loc147)
    %649 = arith.andi %472, %648 : i1
    %650 = arith.cmpi ne, %643, %c1_i32 : i32 loc(#loc148)
    %651 = arith.andi %649, %650 : i1
    %652 = arith.cmpi ne, %643, %c2_i32 : i32 loc(#loc149)
    %653 = arith.andi %651, %652 : i1
    %654 = arith.andi %653, %647 : i1
    acir.path %654 {branch = 94 : i64, kind = "branch", txn = 6 : i64} loc(#loc151)
    acir.path %654 {branch = 94 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    acir.path %654 {branch = 94 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate", "top.rob.slots.3:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %value_96, %ready_97 = acir.read(%654) {branch = 94 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate", "top.rob.slots.3:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc152)
    %655 = acir.extract %value_96 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %656 = acir.aggregate(%true, %false, %641, %638, %c0_i32) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc153)
    %657 = arith.andi %651, %646 : i1
    acir.path %657 {branch = 95 : i64, kind = "branch", txn = 6 : i64} loc(#loc151)
    acir.path %657 {branch = 95 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    acir.path %657 {branch = 95 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate", "top.rob.slots.2:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %value_98, %ready_99 = acir.read(%657) {branch = 95 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate", "top.rob.slots.2:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc152)
    %658 = acir.extract %value_98 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %659 = arith.andi %649, %645 : i1
    acir.path %659 {branch = 96 : i64, kind = "branch", txn = 6 : i64} loc(#loc151)
    acir.path %659 {branch = 96 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    acir.path %659 {branch = 96 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate", "top.rob.slots.1:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %value_100, %ready_101 = acir.read(%659) {branch = 96 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate", "top.rob.slots.1:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc152)
    %660 = acir.extract %value_100 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %661 = arith.andi %472, %644 : i1
    acir.path %661 {branch = 97 : i64, kind = "branch", txn = 6 : i64} loc(#loc151)
    acir.path %661 {branch = 97 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    acir.path %661 {branch = 97 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate", "top.rob.slots.0:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc151)
    %value_102, %ready_103 = acir.read(%661) {branch = 97 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate", "top.rob.slots.0:free"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) loc(#loc152)
    %662 = acir.extract %value_102 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
    %663 = arith.addi %641, %c1_i32 : i32 loc(#loc154)
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc155)
    %664 = arith.cmpi ult, %638, %c8_i32 : i32 loc(#loc156)
    %665 = arith.cmpi ne, %638, %c0_i32 : i32 loc(#loc157)
    "acir.resource.array"() <{name = "top.rf.renamed", resources = ["top.rf.renamed.0", "top.rf.renamed.1", "top.rf.renamed.2", "top.rf.renamed.3", "top.rf.renamed.4", "top.rf.renamed.5", "top.rf.renamed.6", "top.rf.renamed.7"], shape = [8]}> : () -> () loc(#loc114)
    %666 = arith.andi %472, %665 : i1
    %667 = arith.extui %638 : i32 to i64 loc(#loc158)
    %668 = arith.cmpi ult, %667, %c8_i64 : i64 loc(#loc158)
    acir.path %666 {branch = 98 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %669 = arith.cmpi eq, %667, %c0_i64 : i64 loc(#loc158)
    %670 = arith.andi %666, %669 : i1 loc(#loc158)
    acir.path %670 {branch = 99 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %671 = arith.cmpi eq, %667, %c1_i64 : i64 loc(#loc158)
    %672 = arith.andi %666, %671 : i1 loc(#loc158)
    acir.path %672 {branch = 100 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %673 = arith.cmpi eq, %667, %c2_i64 : i64 loc(#loc158)
    %674 = arith.andi %666, %673 : i1 loc(#loc158)
    acir.path %674 {branch = 101 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %675 = arith.cmpi eq, %667, %c3_i64 : i64 loc(#loc158)
    %676 = arith.andi %666, %675 : i1 loc(#loc158)
    acir.path %676 {branch = 102 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %677 = arith.cmpi eq, %667, %c4_i64 : i64 loc(#loc158)
    %678 = arith.andi %666, %677 : i1 loc(#loc158)
    acir.path %678 {branch = 103 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %679 = arith.cmpi eq, %667, %c5_i64 : i64 loc(#loc158)
    %680 = arith.andi %666, %679 : i1 loc(#loc158)
    acir.path %680 {branch = 104 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %681 = arith.cmpi eq, %667, %c6_i64 : i64 loc(#loc158)
    %682 = arith.andi %666, %681 : i1 loc(#loc158)
    acir.path %682 {branch = 105 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %683 = arith.cmpi eq, %667, %c7_i64 : i64 loc(#loc158)
    %684 = arith.andi %666, %683 : i1 loc(#loc158)
    acir.path %684 {branch = 106 : i64, kind = "branch", txn = 6 : i64} loc(#loc158)
    %685 = acir.extract %473 {field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc159)
    %686 = acir.aggregate(%true, %685, %641, %638, %555, %636) : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc160)
    acir.path %472 {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc161)
    %value_104, %ready_105 = acir.read(%472) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.left:peek"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc1)
    %687 = acir.extract %value_104 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc161)
    %688 = arith.xori %687, %true : i1
    %689 = arith.andi %472, %688 : i1
    acir.path %689 {branch = 107 : i64, kind = "branch", txn = 6 : i64} loc(#loc162)
    acir.path %689 {branch = 107 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc162)
    acir.path %689 {branch = 107 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc162)
    %value_106, %ready_107 = acir.read(%689) {branch = 107 : i64, frames = ["top.dispatch:dispatch", "top.left:place", "top.left:free"], method = "read", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc163)
    %690 = acir.extract %value_106 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc163)
    %691 = arith.andi %472, %687 : i1
    acir.path %691 {branch = 108 : i64, kind = "branch", txn = 6 : i64} loc(#loc164)
    acir.path %691 {branch = 108 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc164)
    acir.path %691 {branch = 108 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc164)
    %value_108, %ready_109 = acir.read(%691) {branch = 108 : i64, frames = ["top.dispatch:dispatch", "top.right:place", "top.right:free"], method = "read", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) loc(#loc163)
    %692 = acir.extract %value_108 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc163)
    %693 = arith.xori %662, %true : i1 loc(#loc165)
    %694 = arith.xori %692, %true : i1 loc(#loc165)
    %695 = arith.xori %690, %true : i1 loc(#loc165)
    %696 = arith.select %687, %694, %695 : i1 loc(#loc165)
    %697 = arith.andi %668, %696 : i1 loc(#loc165)
    %698 = arith.select %665, %697, %696 : i1 loc(#loc165)
    %699 = arith.andi %693, %698 : i1 loc(#loc165)
    %700 = arith.xori %660, %true : i1 loc(#loc165)
    %701 = arith.andi %700, %698 : i1 loc(#loc165)
    %702 = arith.xori %658, %true : i1 loc(#loc165)
    %703 = arith.andi %702, %698 : i1 loc(#loc165)
    %704 = arith.xori %655, %true : i1 loc(#loc165)
    %705 = arith.andi %704, %698 : i1 loc(#loc165)
    %706 = arith.andi %647, %705 : i1 loc(#loc165)
    %707 = arith.select %646, %703, %706 : i1 loc(#loc165)
    %708 = arith.select %645, %701, %707 : i1 loc(#loc165)
    %709 = arith.select %644, %699, %708 : i1 loc(#loc165)
    %710 = arith.andi %640, %709 : i1 loc(#loc165)
    %711 = arith.andi %601, %710 : i1 loc(#loc165)
    %712 = arith.select %600, %710, %711 : i1 loc(#loc165)
    %713 = arith.select %599, %710, %712 : i1 loc(#loc165)
    %714 = arith.select %598, %710, %713 : i1 loc(#loc165)
    %715 = arith.select %582, %710, %714 : i1 loc(#loc165)
    %716 = arith.andi %564, %715 : i1 loc(#loc165)
    %717 = arith.select %560, %716, %710 : i1 loc(#loc165)
    %718 = arith.andi %519, %717 : i1 loc(#loc165)
    %719 = arith.select %518, %717, %718 : i1 loc(#loc165)
    %720 = arith.select %517, %717, %719 : i1 loc(#loc165)
    %721 = arith.select %516, %717, %720 : i1 loc(#loc165)
    %722 = arith.select %500, %717, %721 : i1 loc(#loc165)
    %723 = arith.andi %482, %722 : i1 loc(#loc165)
    %724 = arith.select %477, %723, %717 : i1 loc(#loc165)
    %725 = arith.andi %ready_61, %724 : i1 loc(#loc165)
    %726 = arith.andi %value_58, %725 : i1 loc(#loc165)
    %727 = arith.andi %value_56, %726 : i1 loc(#loc165)
    %728 = acir.fire "top.dispatch.dispatch#6" %727 {successors = [], txn = 6 : i64}
    %729 = arith.andi %728, %472 : i1
    acir.effect %729() {branch = 31 : i64, frames = ["top.dispatch:dispatch"], method = "pop", port = 0 : i64, resource = "top.source", txn = 6 : i64} : (i1) -> ()
    acir.effect %729(%475) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc166)
    %730 = arith.andi %477, %501 : i1
    %731 = arith.andi %value_58, %730 : i1
    %732 = arith.andi %value_56, %731 : i1
    %733 = arith.andi %728, %732 : i1
    acir.effect %733(%539) {branch = 58 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %729(%556) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %729(%558) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc166)
    %734 = arith.andi %560, %583 : i1
    %735 = arith.andi %value_58, %734 : i1
    %736 = arith.andi %value_56, %735 : i1
    %737 = arith.andi %728, %736 : i1
    acir.effect %737(%620) {branch = 89 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    acir.effect %729(%637) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %738 = arith.andi %652, %647 : i1
    %739 = arith.andi %650, %738 : i1
    %740 = arith.andi %648, %739 : i1
    %741 = arith.andi %value_58, %740 : i1
    %742 = arith.andi %value_56, %741 : i1
    %743 = arith.andi %728, %742 : i1
    acir.effect %743(%656) {branch = 94 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.3:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc167)
    %744 = arith.andi %650, %646 : i1
    %745 = arith.andi %648, %744 : i1
    %746 = arith.andi %value_58, %745 : i1
    %747 = arith.andi %value_56, %746 : i1
    %748 = arith.andi %728, %747 : i1
    acir.effect %748(%656) {branch = 95 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc167)
    %749 = arith.andi %648, %645 : i1
    %750 = arith.andi %value_58, %749 : i1
    %751 = arith.andi %value_56, %750 : i1
    %752 = arith.andi %728, %751 : i1
    acir.effect %752(%656) {branch = 96 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc167)
    %753 = arith.andi %value_58, %644 : i1
    %754 = arith.andi %value_56, %753 : i1
    %755 = arith.andi %728, %754 : i1
    acir.effect %755(%656) {branch = 97 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate", "top.rob:$dispatch28", "top.rob.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc167)
    acir.effect %729(%663) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rob:allocate"], method = "write", port = 0 : i64, resource = "top.rob.tail", txn = 6 : i64} : (i1, i32) -> () loc(#loc168)
    acir.effect %729(%664) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc169)
    %756 = arith.andi %value_58, %665 : i1
    %757 = arith.andi %value_56, %756 : i1
    %758 = arith.andi %728, %757 : i1
    acir.effect %758(%641, %667) {array = "top.rf.renamed", branch = 98 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.renamed.0", resources = ["top.rf.renamed.0", "top.rf.renamed.1", "top.rf.renamed.2", "top.rf.renamed.3", "top.rf.renamed.4", "top.rf.renamed.5", "top.rf.renamed.6", "top.rf.renamed.7"], txn = 6 : i64} : (i1, i32, i64) -> () loc(#loc158)
    %759 = arith.andi %value_58, %688 : i1
    %760 = arith.andi %value_56, %759 : i1
    %761 = arith.andi %728, %760 : i1
    acir.effect %761(%686) {branch = 107 : i64, frames = ["top.dispatch:dispatch", "top.left:place"], method = "write", port = 0 : i64, resource = "top.left.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc165)
    %762 = arith.andi %value_58, %687 : i1
    %763 = arith.andi %value_56, %762 : i1
    %764 = arith.andi %728, %763 : i1
    acir.effect %764(%686) {branch = 108 : i64, frames = ["top.dispatch:dispatch", "top.right:place"], method = "write", port = 0 : i64, resource = "top.right.slot", txn = 6 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc165)
    acir.path %true {branch = 109 : i64, kind = "branch", txn = 7 : i64} loc(#loc170)
    acir.path %true {branch = 109 : i64, frames = ["top.commit:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc170)
    acir.path %true {branch = 109 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64}
    %value_110, %ready_111 = acir.read(%true) {branch = 109 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "read", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1) -> (i32, i1)
    %765 = acir.bind %value_110 : (i32) -> i32 loc(#loc171)
    %766 = arith.subi %765, %c1_i32 : i32 loc(#loc172)
    %767 = arith.andi %766, %c3_i32 : i32 loc(#loc173)
    %768 = arith.cmpi eq, %767, %c0_i32 : i32 loc(#loc174)
    %769 = arith.cmpi eq, %767, %c1_i32 : i32 loc(#loc175)
    %770 = arith.cmpi eq, %767, %c2_i32 : i32 loc(#loc176)
    %771 = arith.cmpi eq, %767, %c3_i32 : i32 loc(#loc177)
    acir.path %true {branch = 109 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64}
    %772 = arith.cmpi ne, %767, %c0_i32 : i32 loc(#loc174)
    %773 = arith.cmpi ne, %767, %c1_i32 : i32 loc(#loc175)
    %774 = arith.andi %772, %773 : i1
    %775 = arith.cmpi ne, %767, %c2_i32 : i32 loc(#loc176)
    %776 = arith.andi %774, %775 : i1
    %777 = arith.cmpi ne, %767, %c3_i32 : i32 loc(#loc177)
    %778 = arith.andi %776, %771 : i1
    acir.path %778 {branch = 110 : i64, kind = "branch", txn = 7 : i64} loc(#loc178)
    acir.path %778 {branch = 110 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    acir.path %778 {branch = 110 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire", "top.rob.slots.3:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    %value_112, %ready_113 = acir.read(%778) {branch = 110 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire", "top.rob.slots.3:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %779 = acir.bind %value_112 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc179)
    %780 = acir.extract %779 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc180)
    %781 = acir.extract %779 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc181)
    %782 = acir.extract %779 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc182)
    %783 = arith.cmpi eq, %782, %765 : i32 loc(#loc182)
    %value_114, %ready_115 = acir.read(%778) {branch = 110 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %784 = acir.bind %value_114 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc183)
    %785 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc184)
    %786 = arith.select %771, %784, %520 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %787 = arith.andi %774, %770 : i1
    acir.path %787 {branch = 111 : i64, kind = "branch", txn = 7 : i64} loc(#loc178)
    acir.path %787 {branch = 111 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    acir.path %787 {branch = 111 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire", "top.rob.slots.2:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    %value_116, %ready_117 = acir.read(%787) {branch = 111 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire", "top.rob.slots.2:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %788 = acir.bind %value_116 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc185)
    %789 = acir.extract %788 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc180)
    %790 = acir.extract %788 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc181)
    %791 = acir.extract %788 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc182)
    %792 = arith.cmpi eq, %791, %765 : i32 loc(#loc182)
    %value_118, %ready_119 = acir.read(%787) {branch = 111 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %793 = acir.bind %value_118 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc186)
    %794 = arith.andi %775, %777 : i1
    %795 = arith.select %770, %793, %786 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %796 = arith.andi %772, %769 : i1
    acir.path %796 {branch = 112 : i64, kind = "branch", txn = 7 : i64} loc(#loc178)
    acir.path %796 {branch = 112 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    acir.path %796 {branch = 112 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire", "top.rob.slots.1:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    %value_120, %ready_121 = acir.read(%796) {branch = 112 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire", "top.rob.slots.1:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %797 = acir.bind %value_120 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc187)
    %798 = acir.extract %797 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc180)
    %799 = acir.extract %797 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc181)
    %800 = acir.extract %797 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc182)
    %801 = arith.cmpi eq, %800, %765 : i32 loc(#loc182)
    %value_122, %ready_123 = acir.read(%796) {branch = 112 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %802 = acir.bind %value_122 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc188)
    %803 = arith.andi %773, %794 : i1
    %804 = arith.select %769, %802, %795 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.path %768 {branch = 113 : i64, kind = "branch", txn = 7 : i64} loc(#loc178)
    acir.path %768 {branch = 113 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    acir.path %768 {branch = 113 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire", "top.rob.slots.0:completed"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc178)
    %value_124, %ready_125 = acir.read(%768) {branch = 113 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire", "top.rob.slots.0:completed"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %805 = acir.bind %value_124 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc189)
    %806 = acir.extract %805 {field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc180)
    %807 = acir.extract %805 {field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc181)
    %808 = acir.extract %805 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc182)
    %809 = arith.cmpi eq, %808, %765 : i32 loc(#loc182)
    %value_126, %ready_127 = acir.read(%768) {branch = 113 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire"], method = "read", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 7 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1)
    %810 = acir.bind %value_126 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc190)
    %811 = arith.andi %772, %803 : i1
    %812 = arith.select %768, %810, %804 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc191)
    %813 = arith.xori %811, %true : i1
    %814 = arith.addi %765, %c1_i32 : i32 loc(#loc192)
    acir.path %true {branch = 109 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc193)
    %815 = acir.extract %812 {field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc194)
    %816 = arith.cmpi ne, %815, %c0_i32 : i32 loc(#loc194)
    "acir.resource.array"() <{name = "top.rf.registers", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7"], shape = [8]}> : () -> () loc(#loc116)
    "acir.resource.array"() <{name = "top.rf.committed", resources = ["top.rf.committed.0", "top.rf.committed.1", "top.rf.committed.2", "top.rf.committed.3", "top.rf.committed.4", "top.rf.committed.5", "top.rf.committed.6", "top.rf.committed.7"], shape = [8]}> : () -> () loc(#loc115)
    %817 = arith.cmpi eq, %815, %c0_i32 : i32 loc(#loc194)
    %818 = acir.extract %812 {field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc195)
    %819 = arith.extui %815 : i32 to i64 loc(#loc196)
    %820 = arith.cmpi ult, %819, %c8_i64 : i64 loc(#loc196)
    acir.path %816 {branch = 114 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %821 = arith.cmpi eq, %819, %c0_i64 : i64 loc(#loc196)
    %822 = arith.andi %816, %821 : i1 loc(#loc196)
    acir.path %822 {branch = 115 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %823 = arith.cmpi eq, %819, %c1_i64 : i64 loc(#loc196)
    %824 = arith.andi %816, %823 : i1 loc(#loc196)
    acir.path %824 {branch = 116 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %825 = arith.cmpi eq, %819, %c2_i64 : i64 loc(#loc196)
    %826 = arith.andi %816, %825 : i1 loc(#loc196)
    acir.path %826 {branch = 117 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %827 = arith.cmpi eq, %819, %c3_i64 : i64 loc(#loc196)
    %828 = arith.andi %816, %827 : i1 loc(#loc196)
    acir.path %828 {branch = 118 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %829 = arith.cmpi eq, %819, %c4_i64 : i64 loc(#loc196)
    %830 = arith.andi %816, %829 : i1 loc(#loc196)
    acir.path %830 {branch = 119 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %831 = arith.cmpi eq, %819, %c5_i64 : i64 loc(#loc196)
    %832 = arith.andi %816, %831 : i1 loc(#loc196)
    acir.path %832 {branch = 120 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %833 = arith.cmpi eq, %819, %c6_i64 : i64 loc(#loc196)
    %834 = arith.andi %816, %833 : i1 loc(#loc196)
    acir.path %834 {branch = 121 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %835 = arith.cmpi eq, %819, %c7_i64 : i64 loc(#loc196)
    %836 = arith.andi %816, %835 : i1 loc(#loc196)
    acir.path %836 {branch = 122 : i64, kind = "branch", txn = 7 : i64} loc(#loc196)
    %837 = acir.extract %812 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc197)
    acir.path %822 {branch = 123 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    acir.path %824 {branch = 124 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    acir.path %826 {branch = 125 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    acir.path %828 {branch = 126 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    acir.path %830 {branch = 127 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    acir.path %832 {branch = 128 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    acir.path %834 {branch = 129 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    acir.path %836 {branch = 130 : i64, kind = "branch", txn = 7 : i64} loc(#loc198)
    %value_128, %ready_129 = acir.read(%true) {branch = 109 : i64, frames = ["top.commit:commit"], method = "read", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1) -> (i32, i1) loc(#loc199)
    %838 = arith.addi %value_128, %c1_i32 : i32 loc(#loc199)
    %839 = arith.ori %817, %820 : i1 loc(#loc196)
    %840 = arith.andi %809, %839 : i1 loc(#loc196)
    %841 = arith.andi %807, %840 : i1 loc(#loc196)
    %842 = arith.andi %806, %841 : i1 loc(#loc196)
    %843 = arith.andi %801, %839 : i1 loc(#loc196)
    %844 = arith.andi %799, %843 : i1 loc(#loc196)
    %845 = arith.andi %798, %844 : i1 loc(#loc196)
    %846 = arith.andi %792, %839 : i1 loc(#loc196)
    %847 = arith.andi %790, %846 : i1 loc(#loc196)
    %848 = arith.andi %789, %847 : i1 loc(#loc196)
    %849 = arith.andi %783, %839 : i1 loc(#loc196)
    %850 = arith.andi %781, %849 : i1 loc(#loc196)
    %851 = arith.andi %780, %850 : i1 loc(#loc196)
    %852 = arith.andi %771, %851 : i1 loc(#loc196)
    %853 = arith.select %770, %848, %852 : i1 loc(#loc196)
    %854 = arith.select %769, %845, %853 : i1 loc(#loc196)
    %855 = arith.select %768, %842, %854 : i1 loc(#loc196)
    %856 = acir.fire "top.commit.commit#7" %855 {successors = [], txn = 7 : i64}
    %857 = arith.andi %775, %771 : i1
    %858 = arith.andi %773, %857 : i1
    %859 = arith.andi %772, %858 : i1
    %860 = arith.andi %856, %859 : i1
    acir.effect %860(%785) {branch = 110 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.3:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.3.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc200)
    %861 = arith.andi %773, %770 : i1
    %862 = arith.andi %772, %861 : i1
    %863 = arith.andi %856, %862 : i1
    acir.effect %863(%785) {branch = 111 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.2:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.2.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc200)
    %864 = arith.andi %856, %796 : i1
    acir.effect %864(%785) {branch = 112 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.1:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.1.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc200)
    %865 = arith.andi %856, %768 : i1
    acir.effect %865(%785) {branch = 113 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44", "top.rob.slots.0:retire"], method = "write", port = 0 : i64, resource = "top.rob.slots.0.state", txn = 7 : i64} : (i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc200)
    acir.effect %856(%813) {branch = 109 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %856(%814) {branch = 109 : i64, frames = ["top.commit:commit", "top.rob:retire"], method = "write", port = 0 : i64, resource = "top.rob.head", txn = 7 : i64} : (i1, i32) -> () loc(#loc201)
    %866 = arith.andi %856, %816 : i1
    acir.effect %866(%818, %819) {array = "top.rf.registers", branch = 114 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.registers.0", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7"], txn = 7 : i64} : (i1, i32, i64) -> () loc(#loc196)
    acir.effect %866(%837, %819) {array = "top.rf.committed", branch = 114 : i64, frames = ["top.commit:commit", "top.rf:commit"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.committed.0", resources = ["top.rf.committed.0", "top.rf.committed.1", "top.rf.committed.2", "top.rf.committed.3", "top.rf.committed.4", "top.rf.committed.5", "top.rf.committed.6", "top.rf.committed.7"], txn = 7 : i64} : (i1, i32, i64) -> () loc(#loc198)
    acir.effect %856(%838) {branch = 109 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.retired", txn = 7 : i64} : (i1, i32) -> () loc(#loc202)
    acir.effect %856(%837) {branch = 109 : i64, frames = ["top.commit:commit"], method = "write", port = 0 : i64, resource = "top.commit.last_tag", txn = 7 : i64} : (i1, i32) -> () loc(#loc203)
    acir.path %true {branch = 131 : i64, kind = "branch", txn = 8 : i64} loc(#loc204)
    acir.path %true {branch = 131 : i64, frames = ["top.audit:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 8 : i64} loc(#loc204)
    %value_130, %ready_131 = acir.read(%true) {branch = 131 : i64, frames = ["top.audit:observe"], method = "get", port = 0 : i64, resource = "top.event", txn = 8 : i64} : (i1) -> (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1)
    %867 = acir.bind %value_130 : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc205)
    %868 = acir.extract %867 {field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc206)
    %869 = acir.fire "top.audit.observe#8" %ready_131 {successors = [], txn = 8 : i64}
    acir.effect %869(%868) {branch = 131 : i64, frames = ["top.audit:observe"], method = "write", port = 0 : i64, resource = "top.audit.last_tag", txn = 8 : i64} : (i1, i32) -> () loc(#loc207)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":46:16)
#loc2 = loc("acir.binding:top.scheduler_body:a")
#loc3 = loc("acir.binding:top.scheduler_body:b")
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:19)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:31)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:47)
#loc7 = loc("acir.binding:top.scheduler_body:a_ready")
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:19)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc11 = loc("acir.binding:top.scheduler_body:b_ready")
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:29)
#loc13 = loc("acir.binding:top.scheduler_body:a_add")
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc15 = loc("acir.binding:top.scheduler_body:b_add")
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc17 = loc("acir.binding:top.scheduler_body:a_mul")
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc19 = loc("acir.binding:top.scheduler_body:b_mul")
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:38)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:51)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:59)
#loc23 = loc("acir.binding:top.scheduler_body:choose_add_left")
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:38)
#loc25 = loc("acir.binding:top.scheduler_body:choose_mul_left")
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:17)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:20)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:16)
#loc29 = loc("acir.binding:top.scheduler_body.0.call.2.91.0.call.4.17.0.call.4.1:entry")
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:16)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:32)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:55)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:75)
#loc34 = loc("acir.binding:top.scheduler_body.0.call.2.91.0.call.4.17:entry")
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:16)
#loc36 = loc("acir.binding:top.scheduler_body.0.call.2.91.0.call.2.7.0.call.2.1:entry")
#loc37 = loc("acir.binding:top.scheduler_body.0.call.2.91.0.call.2.7:entry")
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:9)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:45)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:20)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:16)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc43 = loc("acir.binding:top.adder_body.0.call.8.2:entry")
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:27)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:38)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:48)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:66)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:16)
#loc49 = loc("acir.binding:top.multiplier_body:busy")
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":79:13)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:19)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:9)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:9)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:9)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:20)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:12)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:25)
#loc58 = loc("acir.binding:top.multiplier_body.0.call.9.4:entry")
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:27)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:38)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:48)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:66)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:16)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:13)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:9)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc67 = loc("acir.binding:top.cdb_body:prefer_add")
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:16)
#loc69 = loc("acir.binding:top.cdb_body.0.call.11.8:result")
#loc70 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19:tag")
#loc71 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19:value")
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:15)
#loc75 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable0")
#loc76 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable1")
#loc77 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable2")
#loc78 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable3")
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:9)
#loc80 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17.0.call.14.27:entry")
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:12)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:33)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:29)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:50)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:13)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":67:13)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:13)
#loc88 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17.0.call.13.25:entry")
#loc89 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17.0.call.12.23:entry")
#loc90 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17.0.call.11.21:entry")
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc92 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.17.22:entry")
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:12)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:20)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:40)
#loc96 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.17.22:changed")
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:29)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:17)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:20)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:40)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:17)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":42:13)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc104 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.18.25:entry")
#loc105 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.18.25:changed")
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc109 = loc("acir.binding:top.dispatch_body.0.call.20.12:inst")
#loc110 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7:index")
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:16)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:12)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:20)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":131:58)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:60)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:60)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:15)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:19)
#loc119 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7:tag")
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:12)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:39)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:20)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:23)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:22)
#loc125 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable0")
#loc126 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable1")
#loc127 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable2")
#loc128 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable3")
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:16)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc131 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7:entry")
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:12)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:28)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:44)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:16)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:39)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:20)
#loc138 = loc("acir.binding:top.dispatch_body.0.call.20.12:lhs")
#loc139 = loc("acir.binding:top.dispatch_body.0.call.20.12:rhs")
#loc140 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13:rd")
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:16)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:23)
#loc144 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13:tag")
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:16)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:15)
#loc147 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable0")
#loc148 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable1")
#loc149 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable2")
#loc150 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable3")
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:9)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:20)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:16)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:16)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:12)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":155:13)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc160 = loc("acir.binding:top.dispatch_body.0.call.20.12:entry")
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:20)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":17:9)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:9)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:9)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:9)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:9)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":70:9)
#loc171 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3:tag")
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:24)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:23)
#loc174 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable0")
#loc175 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable1")
#loc176 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable2")
#loc177 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable3")
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:17)
#loc179 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.50.31.0.call.50.1:entry")
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:16)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:32)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:48)
#loc183 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.50.31:entry")
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:17)
#loc185 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.48.28.0.call.48.1:entry")
#loc186 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.48.28:entry")
#loc187 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.46.25.0.call.46.1:entry")
#loc188 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.46.25:entry")
#loc189 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.44.22.0.call.44.1:entry")
#loc190 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20.0.call.44.22:entry")
#loc191 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3:entry")
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:16)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:9)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":159:12)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":160:35)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":160:13)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:35)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:13)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:19)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:9)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:9)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:9)
#loc205 = loc("acir.binding:top.audit_body.0.call.56.2:saved")
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:20)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:9)
#loc208 = loc(fused[#loc69, #loc67])

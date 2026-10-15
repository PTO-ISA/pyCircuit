module attributes {acir.instances = [{definition = "src_model_3cf90884.OutOfOrder", name = "top", parent, rules = []}, {definition = "src_state_7e811fd9.ReorderBuffer", name = "top.rob", parent = "top", rules = ["allocate", "complete", "has_room", "lookup", "retire", "room"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.0", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.1", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.2", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.3", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}, {definition = "src_state_7e811fd9.RegisterFile", name = "top.rf", parent = "top", rules = ["commit", "operand", "rename"]}, {definition = "src_state_7e811fd9.ReservationStation", name = "top.left", parent = "top", rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}, {definition = "src_state_7e811fd9.ReservationStation", name = "top.right", parent = "top", rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}, {definition = "src_execution_885f3671.Scheduler", name = "top.scheduler", parent = "top", rules = ["issue_add", "issue_mul"]}, {definition = "src_execution_885f3671.Adder", name = "top.adder", parent = "top", rules = ["execute"]}, {definition = "src_execution_885f3671.Multiplier", name = "top.multiplier", parent = "top", rules = ["accept", "advance"]}, {definition = "src_model_3cf90884.Broadcast", name = "top.cdb", parent = "top", rules = ["broadcast"]}, {definition = "src_model_3cf90884.Dispatch", name = "top.dispatch", parent = "top", rules = ["dispatch"]}, {definition = "src_model_3cf90884.Commit", name = "top.commit", parent = "top", rules = ["commit"]}, {definition = "src_model_3cf90884.CompletionAudit", name = "top.audit", parent = "top", rules = ["observe"]}], acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, format_version = 9 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default, name = "lhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "done", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}, acir.stage = "rtl"} {
  %c8_i32 = arith.constant 8 : i32
  %false = arith.constant false
  %c0_i64 = arith.constant 0 : i64
  %c1_i64 = arith.constant 1 : i64
  %c2_i64 = arith.constant 2 : i64
  %c3_i64 = arith.constant 3 : i64
  %c4_i64 = arith.constant 4 : i64
  %c5_i64 = arith.constant 5 : i64
  %c6_i64 = arith.constant 6 : i64
  %c7_i64 = arith.constant 7 : i64
  %c8_i64 = arith.constant 8 : i64
  %c5_i32 = arith.constant 5 : i32
  %c4_i32 = arith.constant 4 : i32
  %true = arith.constant true
  %c3_i32 = arith.constant 3 : i32
  %c2_i32 = arith.constant 2 : i32
  %c1_i32 = arith.constant 1 : i32
  %c0_i32 = arith.constant 0 : i32
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_done", source_branch = 18 : i64, target_branches = [10], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.add_requests", source_branch = 9 : i64, target_branches = [4], to = 0 : i64}, {from = 5 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.event", source_branch = 29 : i64, target_branches = [131], to = 8 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_done", source_branch = 17 : i64, target_branches = [15], to = 4 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mul_requests", source_branch = 11 : i64, target_branches = [8], to = 1 : i64}], logical_states = [{count = "top.add_done$count", kind = "queue", name = "top.add_done", parts = [{path = [0, 0], resource = "top.add_done$storage$0$0"}, {path = [0, 1], resource = "top.add_done$storage$0$1"}, {path = [0, 2], resource = "top.add_done$storage$0$2"}], prototype = [[0 : i32, 0 : i32, 0 : i32]]}, {count = "top.add_requests$count", kind = "queue", name = "top.add_requests", parts = [{path = [0, 0], resource = "top.add_requests$storage$0$0"}, {path = [0, 1], resource = "top.add_requests$storage$0$1"}, {path = [0, 2], resource = "top.add_requests$storage$0$2"}, {path = [0, 3], resource = "top.add_requests$storage$0$3"}, {path = [0, 4, 0], resource = "top.add_requests$storage$0$4$0"}, {path = [0, 4, 1], resource = "top.add_requests$storage$0$4$1"}, {path = [0, 4, 2], resource = "top.add_requests$storage$0$4$2"}, {path = [0, 5, 0], resource = "top.add_requests$storage$0$5$0"}, {path = [0, 5, 1], resource = "top.add_requests$storage$0$5$1"}, {path = [0, 5, 2], resource = "top.add_requests$storage$0$5$2"}], prototype = [[false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]]]}, {kind = "cell", name = "top.audit.last_tag", parts = [{path = [], resource = "top.audit.last_tag$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.commit.last_tag", parts = [{path = [], resource = "top.commit.last_tag$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.commit.retired", parts = [{path = [], resource = "top.commit.retired$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.left.slot", parts = [{path = [0], resource = "top.left.slot$value$0"}, {path = [1], resource = "top.left.slot$value$1"}, {path = [2], resource = "top.left.slot$value$2"}, {path = [3], resource = "top.left.slot$value$3"}, {path = [4, 0], resource = "top.left.slot$value$4$0"}, {path = [4, 1], resource = "top.left.slot$value$4$1"}, {path = [4, 2], resource = "top.left.slot$value$4$2"}, {path = [5, 0], resource = "top.left.slot$value$5$0"}, {path = [5, 1], resource = "top.left.slot$value$5$1"}, {path = [5, 2], resource = "top.left.slot$value$5$2"}], prototype = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]]}, {count = "top.mul_done$count", kind = "queue", name = "top.mul_done", parts = [{path = [0, 0], resource = "top.mul_done$storage$0$0"}, {path = [0, 1], resource = "top.mul_done$storage$0$1"}, {path = [0, 2], resource = "top.mul_done$storage$0$2"}], prototype = [[0 : i32, 0 : i32, 0 : i32]]}, {count = "top.mul_requests$count", kind = "queue", name = "top.mul_requests", parts = [{path = [0, 0], resource = "top.mul_requests$storage$0$0"}, {path = [0, 1], resource = "top.mul_requests$storage$0$1"}, {path = [0, 2], resource = "top.mul_requests$storage$0$2"}, {path = [0, 3], resource = "top.mul_requests$storage$0$3"}, {path = [0, 4, 0], resource = "top.mul_requests$storage$0$4$0"}, {path = [0, 4, 1], resource = "top.mul_requests$storage$0$4$1"}, {path = [0, 4, 2], resource = "top.mul_requests$storage$0$4$2"}, {path = [0, 5, 0], resource = "top.mul_requests$storage$0$5$0"}, {path = [0, 5, 1], resource = "top.mul_requests$storage$0$5$1"}, {path = [0, 5, 2], resource = "top.mul_requests$storage$0$5$2"}], prototype = [[false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]]]}, {kind = "cell", name = "top.multiplier.active", parts = [{path = [], resource = "top.multiplier.active$value"}], prototype = false}, {kind = "cell", name = "top.multiplier.remaining", parts = [{path = [], resource = "top.multiplier.remaining$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.multiplier.request", parts = [{path = [0], resource = "top.multiplier.request$value$0"}, {path = [1], resource = "top.multiplier.request$value$1"}, {path = [2], resource = "top.multiplier.request$value$2"}, {path = [3], resource = "top.multiplier.request$value$3"}, {path = [4, 0], resource = "top.multiplier.request$value$4$0"}, {path = [4, 1], resource = "top.multiplier.request$value$4$1"}, {path = [4, 2], resource = "top.multiplier.request$value$4$2"}, {path = [5, 0], resource = "top.multiplier.request$value$5$0"}, {path = [5, 1], resource = "top.multiplier.request$value$5$1"}, {path = [5, 2], resource = "top.multiplier.request$value$5$2"}], prototype = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]]}, {kind = "cell", name = "top.rf.committed.0", parts = [{path = [], resource = "top.rf.committed.0$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.committed.1", parts = [{path = [], resource = "top.rf.committed.1$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.committed.2", parts = [{path = [], resource = "top.rf.committed.2$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.committed.3", parts = [{path = [], resource = "top.rf.committed.3$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.committed.4", parts = [{path = [], resource = "top.rf.committed.4$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.committed.5", parts = [{path = [], resource = "top.rf.committed.5$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.committed.6", parts = [{path = [], resource = "top.rf.committed.6$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.committed.7", parts = [{path = [], resource = "top.rf.committed.7$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.registers.0", parts = [{path = [], resource = "top.rf.registers.0$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.registers.1", parts = [{path = [], resource = "top.rf.registers.1$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.registers.2", parts = [{path = [], resource = "top.rf.registers.2$value"}], prototype = 6 : i32}, {kind = "cell", name = "top.rf.registers.3", parts = [{path = [], resource = "top.rf.registers.3$value"}], prototype = 7 : i32}, {kind = "cell", name = "top.rf.registers.4", parts = [{path = [], resource = "top.rf.registers.4$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.registers.5", parts = [{path = [], resource = "top.rf.registers.5$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.registers.6", parts = [{path = [], resource = "top.rf.registers.6$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.registers.7", parts = [{path = [], resource = "top.rf.registers.7$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.0", parts = [{path = [], resource = "top.rf.renamed.0$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.1", parts = [{path = [], resource = "top.rf.renamed.1$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.2", parts = [{path = [], resource = "top.rf.renamed.2$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.3", parts = [{path = [], resource = "top.rf.renamed.3$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.4", parts = [{path = [], resource = "top.rf.renamed.4$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.5", parts = [{path = [], resource = "top.rf.renamed.5$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.6", parts = [{path = [], resource = "top.rf.renamed.6$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.rf.renamed.7", parts = [{path = [], resource = "top.rf.renamed.7$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.right.slot", parts = [{path = [0], resource = "top.right.slot$value$0"}, {path = [1], resource = "top.right.slot$value$1"}, {path = [2], resource = "top.right.slot$value$2"}, {path = [3], resource = "top.right.slot$value$3"}, {path = [4, 0], resource = "top.right.slot$value$4$0"}, {path = [4, 1], resource = "top.right.slot$value$4$1"}, {path = [4, 2], resource = "top.right.slot$value$4$2"}, {path = [5, 0], resource = "top.right.slot$value$5$0"}, {path = [5, 1], resource = "top.right.slot$value$5$1"}, {path = [5, 2], resource = "top.right.slot$value$5$2"}], prototype = [false, 0 : i32, 0 : i32, 0 : i32, [false, 0 : i32, 0 : i32], [false, 0 : i32, 0 : i32]]}, {kind = "cell", name = "top.rob.head", parts = [{path = [], resource = "top.rob.head$value"}], prototype = 1 : i32}, {kind = "cell", name = "top.rob.slots.0.state", parts = [{path = [0], resource = "top.rob.slots.0.state$value$0"}, {path = [1], resource = "top.rob.slots.0.state$value$1"}, {path = [2], resource = "top.rob.slots.0.state$value$2"}, {path = [3], resource = "top.rob.slots.0.state$value$3"}, {path = [4], resource = "top.rob.slots.0.state$value$4"}], prototype = [false, false, 0 : i32, 0 : i32, 0 : i32]}, {kind = "cell", name = "top.rob.slots.1.state", parts = [{path = [0], resource = "top.rob.slots.1.state$value$0"}, {path = [1], resource = "top.rob.slots.1.state$value$1"}, {path = [2], resource = "top.rob.slots.1.state$value$2"}, {path = [3], resource = "top.rob.slots.1.state$value$3"}, {path = [4], resource = "top.rob.slots.1.state$value$4"}], prototype = [false, false, 0 : i32, 0 : i32, 0 : i32]}, {kind = "cell", name = "top.rob.slots.2.state", parts = [{path = [0], resource = "top.rob.slots.2.state$value$0"}, {path = [1], resource = "top.rob.slots.2.state$value$1"}, {path = [2], resource = "top.rob.slots.2.state$value$2"}, {path = [3], resource = "top.rob.slots.2.state$value$3"}, {path = [4], resource = "top.rob.slots.2.state$value$4"}], prototype = [false, false, 0 : i32, 0 : i32, 0 : i32]}, {kind = "cell", name = "top.rob.slots.3.state", parts = [{path = [0], resource = "top.rob.slots.3.state$value$0"}, {path = [1], resource = "top.rob.slots.3.state$value$1"}, {path = [2], resource = "top.rob.slots.3.state$value$2"}, {path = [3], resource = "top.rob.slots.3.state$value$3"}, {path = [4], resource = "top.rob.slots.3.state$value$4"}], prototype = [false, false, 0 : i32, 0 : i32, 0 : i32]}, {kind = "cell", name = "top.rob.tail", parts = [{path = [], resource = "top.rob.tail$value"}], prototype = 1 : i32}, {count = "top.source$count", kind = "queue", name = "top.source", parts = [{path = [0, 0], resource = "top.source$storage$0$0"}, {path = [0, 1], resource = "top.source$storage$0$1"}, {path = [0, 2], resource = "top.source$storage$0$2"}, {path = [0, 3], resource = "top.source$storage$0$3"}, {path = [1, 0], resource = "top.source$storage$1$0"}, {path = [1, 1], resource = "top.source$storage$1$1"}, {path = [1, 2], resource = "top.source$storage$1$2"}, {path = [1, 3], resource = "top.source$storage$1$3"}, {path = [2, 0], resource = "top.source$storage$2$0"}, {path = [2, 1], resource = "top.source$storage$2$1"}, {path = [2, 2], resource = "top.source$storage$2$2"}, {path = [2, 3], resource = "top.source$storage$2$3"}, {path = [3, 0], resource = "top.source$storage$3$0"}, {path = [3, 1], resource = "top.source$storage$3$1"}, {path = [3, 2], resource = "top.source$storage$3$2"}, {path = [3, 3], resource = "top.source$storage$3$3"}], prototype = [[1 : i32, 1 : i32, 2 : i32, 3 : i32], [0 : i32, 4 : i32, 1 : i32, 2 : i32], [0 : i32, 5 : i32, 2 : i32, 3 : i32], [0 : i32, 6 : i32, 4 : i32, 5 : i32]]}], schedule = [3, 1, 5, 2, 0, 4, 6, 7, 8], stage = "rtl"} {
    acir.state @top.add_done$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_done$storage$0$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_done$storage$0$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_done$storage$0$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$storage$0$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.add_requests$storage$0$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$storage$0$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$storage$0$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$storage$0$4$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.add_requests$storage$0$4$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$storage$0$4$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$storage$0$5$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.add_requests$storage$0$5$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.add_requests$storage$0$5$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.audit.last_tag$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.commit.last_tag$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.commit.retired$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.left.slot$value$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot$value$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot$value$4$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.left.slot$value$4$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot$value$4$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot$value$5$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.left.slot$value$5$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left.slot$value$5$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_done$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_done$storage$0$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_done$storage$0$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_done$storage$0$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$storage$0$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.mul_requests$storage$0$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$storage$0$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$storage$0$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$storage$0$4$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.mul_requests$storage$0$4$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$storage$0$4$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$storage$0$5$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.mul_requests$storage$0$5$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.mul_requests$storage$0$5$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.active$value : i1 {initial = false, kind = "cell"}
    acir.state @top.multiplier.remaining$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.multiplier.request$value$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request$value$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request$value$4$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.multiplier.request$value$4$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request$value$4$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request$value$5$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.multiplier.request$value$5$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.multiplier.request$value$5$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.0$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.1$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.2$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.3$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.4$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.5$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.6$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.committed.7$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.0$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.1$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.2$value : i32 {initial = 6 : i32, kind = "cell"}
    acir.state @top.rf.registers.3$value : i32 {initial = 7 : i32, kind = "cell"}
    acir.state @top.rf.registers.4$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.5$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.6$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.7$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.0$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.1$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.2$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.3$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.4$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.5$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.6$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.renamed.7$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.right.slot$value$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot$value$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot$value$4$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.right.slot$value$4$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot$value$4$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot$value$5$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.right.slot$value$5$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right.slot$value$5$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.head$value : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.rob.slots.0.state$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.0.state$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.0.state$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.0.state$value$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.0.state$value$4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.1.state$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.1.state$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.1.state$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.1.state$value$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.1.state$value$4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.2.state$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.2.state$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.2.state$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.2.state$value$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.2.state$value$4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.3.state$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.3.state$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.rob.slots.3.state$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.3.state$value$3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.slots.3.state$value$4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rob.tail$value : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source$count : i32 {initial = 4 : i32, kind = "cell"}
    acir.state @top.source$storage$0$0 : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source$storage$0$1 : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source$storage$0$2 : i32 {initial = 2 : i32, kind = "cell"}
    acir.state @top.source$storage$0$3 : i32 {initial = 3 : i32, kind = "cell"}
    acir.state @top.source$storage$1$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.source$storage$1$1 : i32 {initial = 4 : i32, kind = "cell"}
    acir.state @top.source$storage$1$2 : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source$storage$1$3 : i32 {initial = 2 : i32, kind = "cell"}
    acir.state @top.source$storage$2$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.source$storage$2$1 : i32 {initial = 5 : i32, kind = "cell"}
    acir.state @top.source$storage$2$2 : i32 {initial = 2 : i32, kind = "cell"}
    acir.state @top.source$storage$2$3 : i32 {initial = 3 : i32, kind = "cell"}
    acir.state @top.source$storage$3$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.source$storage$3$1 : i32 {initial = 6 : i32, kind = "cell"}
    acir.state @top.source$storage$3$2 : i32 {initial = 4 : i32, kind = "cell"}
    acir.state @top.source$storage$3$3 : i32 {initial = 5 : i32, kind = "cell"}
    %0 = acir.current {aspect = "value", resource = "top.add_done$count"} : i32
    %1 = arith.cmpi eq, %0, %c0_i32 : i32 loc(#loc1)
    %2 = acir.current {aspect = "value", resource = "top.mul_done$count"} : i32
    %3 = arith.cmpi ne, %2, %c0_i32 : i32
    %4 = acir.current {aspect = "value", resource = "top.mul_done$storage$0$0"} : i32
    %5 = acir.current {aspect = "value", resource = "top.mul_done$storage$0$1"} : i32
    %6 = acir.current {aspect = "value", resource = "top.mul_done$storage$0$2"} : i32
    %7 = acir.current {aspect = "value", resource = "top.add_done$storage$0$0"} : i32
    %8 = acir.current {aspect = "value", resource = "top.add_done$storage$0$1"} : i32
    %9 = acir.current {aspect = "value", resource = "top.add_done$storage$0$2"} : i32
    %10 = arith.select %1, %4, %7 : i32
    %11 = arith.select %1, %6, %9 : i32
    %12 = arith.subi %10, %c1_i32 : i32
    %13 = arith.andi %12, %c3_i32 : i32
    %14 = arith.cmpi eq, %13, %c0_i32 : i32
    %15 = arith.cmpi eq, %13, %c1_i32 : i32 loc(#loc2)
    %16 = arith.cmpi eq, %13, %c2_i32 : i32
    %17 = arith.cmpi eq, %13, %c3_i32 : i32 loc(#loc3)
    %18 = arith.ori %16, %17 : i1 loc(#loc3)
    %19 = arith.ori %15, %18 : i1 loc(#loc3)
    %20 = arith.ori %14, %19 : i1 loc(#loc3)
    %21 = arith.andi %3, %20 : i1 loc(#loc3)
    %22 = arith.select %1, %21, %20 : i1 loc(#loc3)
    %23 = acir.fire "top.cdb.broadcast#5" %22 {successors = [2, 4, 8], txn = 5 : i64} loc(#loc3)
    %24 = acir.current {aspect = "value", resource = "top.add_requests$count"} : i32
    %25 = arith.cmpi ne, %24, %c0_i32 : i32
    %26 = arith.cmpi ne, %0, %c0_i32 : i32 loc(#loc1)
    %27 = arith.andi %23, %26 : i1
    %28 = arith.extui %27 : i1 to i32
    %29 = arith.subi %0, %28 : i32
    %30 = arith.cmpi ult, %29, %c1_i32 : i32
    %31 = arith.andi %25, %30 : i1
    %32 = acir.fire "top.adder.execute#2" %31 {successors = [0], txn = 2 : i64}
    %33 = acir.current {aspect = "value", resource = "top.left.slot$value$0"} : i1 loc(#loc4)
    %34 = acir.current {aspect = "value", resource = "top.left.slot$value$1"} : i32 loc(#loc4)
    %35 = acir.current {aspect = "value", resource = "top.left.slot$value$2"} : i32 loc(#loc4)
    %36 = acir.current {aspect = "value", resource = "top.left.slot$value$3"} : i32 loc(#loc4)
    %37 = acir.current {aspect = "value", resource = "top.left.slot$value$4$0"} : i1 loc(#loc4)
    %38 = acir.current {aspect = "value", resource = "top.left.slot$value$4$1"} : i32 loc(#loc4)
    %39 = acir.current {aspect = "value", resource = "top.left.slot$value$4$2"} : i32 loc(#loc4)
    %40 = acir.aggregate(%37, %38, %39) : (i1, i32, i32) -> tuple<i1, i32, i32> loc(#loc4)
    %41 = acir.current {aspect = "value", resource = "top.left.slot$value$5$0"} : i1 loc(#loc4)
    %42 = acir.current {aspect = "value", resource = "top.left.slot$value$5$1"} : i32 loc(#loc4)
    %43 = acir.current {aspect = "value", resource = "top.left.slot$value$5$2"} : i32 loc(#loc4)
    %44 = acir.aggregate(%41, %42, %43) : (i1, i32, i32) -> tuple<i1, i32, i32> loc(#loc4)
    %45 = acir.aggregate(%33, %34, %35, %36, %40, %44) : (i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>) -> tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>
    %46 = acir.export %45 : tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>> -> tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>> loc(#loc4)
    %47 = acir.extract %46[%c0_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> i1 loc(#loc4)
    %48 = acir.extract %46[%c4_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> tuple<i1, i32, i32> loc(#loc5)
    %49 = acir.extract %48[%c0_i32] : (tuple<i1, i32, i32>, i32) -> i1 loc(#loc5)
    %50 = acir.extract %46[%c5_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> tuple<i1, i32, i32> loc(#loc6)
    %51 = acir.extract %50[%c0_i32] : (tuple<i1, i32, i32>, i32) -> i1 loc(#loc6)
    %52 = acir.extract %46[%c1_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> i32 loc(#loc7)
    %53 = arith.cmpi eq, %52, %c0_i32 : i32 loc(#loc7)
    %54 = acir.current {aspect = "value", resource = "top.right.slot$value$0"} : i1 loc(#loc4)
    %55 = acir.current {aspect = "value", resource = "top.right.slot$value$1"} : i32 loc(#loc4)
    %56 = acir.current {aspect = "value", resource = "top.right.slot$value$2"} : i32 loc(#loc4)
    %57 = acir.current {aspect = "value", resource = "top.right.slot$value$3"} : i32 loc(#loc4)
    %58 = acir.current {aspect = "value", resource = "top.right.slot$value$4$0"} : i1 loc(#loc4)
    %59 = acir.current {aspect = "value", resource = "top.right.slot$value$4$1"} : i32 loc(#loc4)
    %60 = acir.current {aspect = "value", resource = "top.right.slot$value$4$2"} : i32 loc(#loc4)
    %61 = acir.aggregate(%58, %59, %60) : (i1, i32, i32) -> tuple<i1, i32, i32> loc(#loc4)
    %62 = acir.current {aspect = "value", resource = "top.right.slot$value$5$0"} : i1 loc(#loc4)
    %63 = acir.current {aspect = "value", resource = "top.right.slot$value$5$1"} : i32 loc(#loc4)
    %64 = acir.current {aspect = "value", resource = "top.right.slot$value$5$2"} : i32 loc(#loc4)
    %65 = acir.aggregate(%62, %63, %64) : (i1, i32, i32) -> tuple<i1, i32, i32> loc(#loc4)
    %66 = acir.aggregate(%54, %55, %56, %57, %61, %65) : (i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>) -> tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>
    %67 = acir.export %66 : tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>> -> tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>> loc(#loc4)
    %68 = acir.extract %67[%c0_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> i1 loc(#loc4)
    %69 = acir.extract %67[%c4_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> tuple<i1, i32, i32> loc(#loc8)
    %70 = acir.extract %69[%c0_i32] : (tuple<i1, i32, i32>, i32) -> i1 loc(#loc8)
    %71 = acir.extract %67[%c5_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> tuple<i1, i32, i32> loc(#loc9)
    %72 = acir.extract %71[%c0_i32] : (tuple<i1, i32, i32>, i32) -> i1 loc(#loc9)
    %73 = acir.extract %67[%c1_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> i32 loc(#loc10)
    %74 = arith.cmpi eq, %73, %c0_i32 : i32 loc(#loc10)
    %75 = acir.extract %46[%c2_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> i32 loc(#loc11)
    %76 = acir.extract %67[%c2_i32] : (tuple<i1, i32, i32, i32, tuple<i1, i32, i32>, tuple<i1, i32, i32>>, i32) -> i32 loc(#loc12)
    %77 = arith.cmpi ult, %75, %76 : i32 loc(#loc12)
    %78 = arith.cmpi eq, %34, %c0_i32 : i32 loc(#loc13)
    %79 = arith.extui %32 : i1 to i32 loc(#loc14)
    %80 = arith.subi %24, %79 : i32 loc(#loc14)
    %81 = arith.cmpi ult, %80, %c1_i32 : i32
    %82 = arith.cmpi eq, %55, %c0_i32 : i32 loc(#loc13)
    %83 = arith.andi %41, %81 : i1 loc(#loc15)
    %84 = arith.andi %37, %83 : i1 loc(#loc15)
    %85 = arith.andi %78, %84 : i1 loc(#loc15)
    %86 = arith.andi %33, %85 : i1 loc(#loc15)
    %87 = arith.andi %58, %62 : i1 loc(#loc15)
    %88 = arith.andi %82, %87 : i1 loc(#loc15)
    %89 = arith.andi %54, %88 : i1 loc(#loc15)
    %90 = arith.andi %81, %89 : i1 loc(#loc15)
    %91 = arith.select %77, %86, %90 : i1 loc(#loc15)
    %92 = arith.select %74, %91, %86 : i1 loc(#loc15)
    %93 = arith.select %72, %92, %86 : i1 loc(#loc15)
    %94 = arith.select %70, %93, %86 : i1 loc(#loc15)
    %95 = arith.select %68, %94, %86 : i1 loc(#loc15)
    %96 = arith.select %53, %95, %90 : i1 loc(#loc15)
    %97 = arith.select %51, %96, %90 : i1 loc(#loc15)
    %98 = arith.select %49, %97, %90 : i1 loc(#loc15)
    %99 = arith.select %47, %98, %90 : i1 loc(#loc15)
    %100 = acir.fire "top.scheduler.issue_add#0" %99 {successors = [], txn = 0 : i64} loc(#loc15)
    %101 = acir.current {aspect = "value", resource = "top.multiplier.active$value"} : i1
    %102 = arith.xori %101, %true : i1 loc(#loc16)
    %103 = acir.current {aspect = "value", resource = "top.mul_requests$count"} : i32
    %104 = arith.cmpi ne, %103, %c0_i32 : i32 loc(#loc16)
    %105 = arith.andi %102, %104 : i1 loc(#loc16)
    %106 = acir.fire "top.multiplier.accept#3" %105 {successors = [1], txn = 3 : i64} loc(#loc16)
    %107 = arith.cmpi eq, %52, %c1_i32 : i32 loc(#loc17)
    %108 = arith.cmpi eq, %73, %c1_i32 : i32 loc(#loc18)
    %109 = arith.cmpi eq, %34, %c1_i32 : i32 loc(#loc13)
    %110 = arith.andi %106, %102 : i1 loc(#loc16)
    %111 = arith.extui %110 : i1 to i32 loc(#loc16)
    %112 = arith.subi %103, %111 : i32 loc(#loc16)
    %113 = arith.cmpi ult, %112, %c1_i32 : i32 loc(#loc16)
    %114 = arith.cmpi eq, %55, %c1_i32 : i32 loc(#loc13)
    %115 = arith.andi %41, %113 : i1 loc(#loc15)
    %116 = arith.andi %37, %115 : i1 loc(#loc15)
    %117 = arith.andi %109, %116 : i1 loc(#loc15)
    %118 = arith.andi %33, %117 : i1 loc(#loc15)
    %119 = arith.andi %114, %87 : i1 loc(#loc15)
    %120 = arith.andi %54, %119 : i1 loc(#loc15)
    %121 = arith.andi %113, %120 : i1 loc(#loc15)
    %122 = arith.select %77, %118, %121 : i1 loc(#loc15)
    %123 = arith.select %108, %122, %118 : i1 loc(#loc15)
    %124 = arith.select %72, %123, %118 : i1 loc(#loc15)
    %125 = arith.select %70, %124, %118 : i1 loc(#loc15)
    %126 = arith.select %68, %125, %118 : i1 loc(#loc15)
    %127 = arith.select %107, %126, %121 : i1 loc(#loc15)
    %128 = arith.select %51, %127, %121 : i1 loc(#loc15)
    %129 = arith.select %49, %128, %121 : i1 loc(#loc15)
    %130 = arith.select %47, %129, %121 : i1 loc(#loc15)
    %131 = acir.fire "top.scheduler.issue_mul#1" %130 {successors = [], txn = 1 : i64} loc(#loc15)
    %132 = acir.current {aspect = "value", resource = "top.multiplier.remaining$value"} : i32 loc(#loc19)
    %133 = arith.cmpi ne, %132, %c0_i32 : i32 loc(#loc19)
    %134 = arith.andi %23, %1 : i1
    %135 = arith.extui %134 : i1 to i32 loc(#loc20)
    %136 = arith.subi %2, %135 : i32 loc(#loc20)
    %137 = arith.cmpi ult, %136, %c1_i32 : i32
    %138 = arith.ori %133, %137 : i1
    %139 = arith.andi %101, %138 : i1
    %140 = acir.fire "top.multiplier.advance#4" %139 {successors = [], txn = 4 : i64}
    %141 = arith.cmpi eq, %2, %c0_i32 : i32 loc(#loc21)
    %142 = acir.current {aspect = "value", resource = "top.source$count"} : i32
    %143 = arith.cmpi ne, %142, %c0_i32 : i32
    %144 = acir.current {aspect = "value", resource = "top.source$storage$0$0"} : i32
    %145 = acir.current {aspect = "value", resource = "top.source$storage$0$1"} : i32
    %146 = acir.current {aspect = "value", resource = "top.source$storage$0$2"} : i32
    %147 = acir.current {aspect = "value", resource = "top.source$storage$0$3"} : i32
    %148 = acir.current {aspect = "value", resource = "top.source$storage$1$0"} : i32
    %149 = acir.current {aspect = "value", resource = "top.source$storage$1$1"} : i32
    %150 = acir.current {aspect = "value", resource = "top.source$storage$1$2"} : i32
    %151 = acir.current {aspect = "value", resource = "top.source$storage$1$3"} : i32
    %152 = acir.current {aspect = "value", resource = "top.source$storage$2$0"} : i32
    %153 = acir.current {aspect = "value", resource = "top.source$storage$2$1"} : i32
    %154 = acir.current {aspect = "value", resource = "top.source$storage$2$2"} : i32
    %155 = acir.current {aspect = "value", resource = "top.source$storage$2$3"} : i32
    %156 = acir.current {aspect = "value", resource = "top.source$storage$3$0"} : i32
    %157 = acir.current {aspect = "value", resource = "top.source$storage$3$1"} : i32
    %158 = acir.current {aspect = "value", resource = "top.source$storage$3$2"} : i32
    %159 = acir.current {aspect = "value", resource = "top.source$storage$3$3"} : i32
    %160 = arith.cmpi ne, %146, %c0_i32 : i32
    %161 = arith.extui %146 : i32 to i64 loc(#loc22)
    %162 = arith.cmpi ult, %161, %c8_i64 : i64
    %163 = arith.cmpi eq, %161, %c7_i64 : i64 loc(#loc23)
    %164 = acir.current {aspect = "value", resource = "top.rf.renamed.7$value"} : i32
    %165 = arith.cmpi eq, %161, %c6_i64 : i64 loc(#loc23)
    %166 = acir.current {aspect = "value", resource = "top.rf.renamed.6$value"} : i32
    %167 = arith.cmpi eq, %161, %c5_i64 : i64 loc(#loc23)
    %168 = acir.current {aspect = "value", resource = "top.rf.renamed.5$value"} : i32
    %169 = arith.cmpi eq, %161, %c4_i64 : i64 loc(#loc23)
    %170 = acir.current {aspect = "value", resource = "top.rf.renamed.4$value"} : i32
    %171 = arith.cmpi eq, %161, %c3_i64 : i64 loc(#loc23)
    %172 = acir.current {aspect = "value", resource = "top.rf.renamed.3$value"} : i32
    %173 = arith.cmpi eq, %161, %c2_i64 : i64 loc(#loc23)
    %174 = acir.current {aspect = "value", resource = "top.rf.renamed.2$value"} : i32
    %175 = arith.cmpi eq, %161, %c1_i64 : i64 loc(#loc23)
    %176 = acir.current {aspect = "value", resource = "top.rf.renamed.1$value"} : i32
    %177 = arith.cmpi eq, %161, %c0_i64 : i64 loc(#loc23)
    %178 = acir.current {aspect = "value", resource = "top.rf.renamed.0$value"} : i32
    %179 = arith.select %177, %178, %c0_i32 : i32
    %180 = arith.select %175, %176, %179 : i32
    %181 = arith.select %173, %174, %180 : i32
    %182 = arith.select %171, %172, %181 : i32
    %183 = arith.select %169, %170, %182 : i32
    %184 = arith.select %167, %168, %183 : i32
    %185 = arith.select %165, %166, %184 : i32
    %186 = arith.select %163, %164, %185 : i32
    %187 = acir.current {aspect = "value", resource = "top.rf.committed.7$value"} : i32 loc(#loc23)
    %188 = acir.current {aspect = "value", resource = "top.rf.committed.6$value"} : i32 loc(#loc23)
    %189 = acir.current {aspect = "value", resource = "top.rf.committed.5$value"} : i32 loc(#loc23)
    %190 = acir.current {aspect = "value", resource = "top.rf.committed.4$value"} : i32 loc(#loc23)
    %191 = acir.current {aspect = "value", resource = "top.rf.committed.3$value"} : i32 loc(#loc23)
    %192 = acir.current {aspect = "value", resource = "top.rf.committed.2$value"} : i32 loc(#loc23)
    %193 = acir.current {aspect = "value", resource = "top.rf.committed.1$value"} : i32 loc(#loc23)
    %194 = acir.current {aspect = "value", resource = "top.rf.committed.0$value"} : i32 loc(#loc23)
    %195 = arith.select %177, %194, %c0_i32 : i32 loc(#loc23)
    %196 = arith.select %175, %193, %195 : i32 loc(#loc23)
    %197 = arith.select %173, %192, %196 : i32 loc(#loc23)
    %198 = arith.select %171, %191, %197 : i32 loc(#loc23)
    %199 = arith.select %169, %190, %198 : i32 loc(#loc23)
    %200 = arith.select %167, %189, %199 : i32 loc(#loc23)
    %201 = arith.select %165, %188, %200 : i32 loc(#loc23)
    %202 = arith.select %163, %187, %201 : i32 loc(#loc23)
    %203 = arith.cmpi eq, %186, %202 : i32 loc(#loc23)
    %204 = arith.cmpi ne, %147, %c0_i32 : i32 loc(#loc24)
    %205 = arith.extui %147 : i32 to i64 loc(#loc22)
    %206 = arith.cmpi ult, %205, %c8_i64 : i64 loc(#loc22)
    %207 = arith.cmpi eq, %205, %c7_i64 : i64 loc(#loc23)
    %208 = arith.cmpi eq, %205, %c6_i64 : i64 loc(#loc23)
    %209 = arith.cmpi eq, %205, %c5_i64 : i64 loc(#loc23)
    %210 = arith.cmpi eq, %205, %c4_i64 : i64 loc(#loc23)
    %211 = arith.cmpi eq, %205, %c3_i64 : i64 loc(#loc23)
    %212 = arith.cmpi eq, %205, %c2_i64 : i64 loc(#loc23)
    %213 = arith.cmpi eq, %205, %c1_i64 : i64 loc(#loc23)
    %214 = arith.cmpi eq, %205, %c0_i64 : i64 loc(#loc23)
    %215 = arith.select %214, %178, %c0_i32 : i32
    %216 = arith.select %213, %176, %215 : i32
    %217 = arith.select %212, %174, %216 : i32
    %218 = arith.select %211, %172, %217 : i32
    %219 = arith.select %210, %170, %218 : i32
    %220 = arith.select %209, %168, %219 : i32
    %221 = arith.select %208, %166, %220 : i32
    %222 = arith.select %207, %164, %221 : i32
    %223 = arith.select %214, %194, %c0_i32 : i32 loc(#loc23)
    %224 = arith.select %213, %193, %223 : i32 loc(#loc23)
    %225 = arith.select %212, %192, %224 : i32 loc(#loc23)
    %226 = arith.select %211, %191, %225 : i32 loc(#loc23)
    %227 = arith.select %210, %190, %226 : i32 loc(#loc23)
    %228 = arith.select %209, %189, %227 : i32 loc(#loc23)
    %229 = arith.select %208, %188, %228 : i32 loc(#loc23)
    %230 = arith.select %207, %187, %229 : i32 loc(#loc23)
    %231 = arith.cmpi eq, %222, %230 : i32 loc(#loc23)
    %232 = acir.current {aspect = "value", resource = "top.rob.tail$value"} : i32
    %233 = acir.current {aspect = "value", resource = "top.rob.head$value"} : i32
    %234 = arith.subi %232, %233 : i32 loc(#loc25)
    %235 = arith.cmpi ult, %234, %c4_i32 : i32
    %236 = arith.subi %232, %c1_i32 : i32
    %237 = arith.andi %236, %c3_i32 : i32
    %238 = arith.cmpi eq, %237, %c0_i32 : i32
    %239 = acir.current {aspect = "value", resource = "top.rob.slots.0.state$value$0"} : i1 loc(#loc26)
    %240 = acir.current {aspect = "value", resource = "top.rob.slots.0.state$value$1"} : i1 loc(#loc26)
    %241 = acir.current {aspect = "value", resource = "top.rob.slots.0.state$value$2"} : i32 loc(#loc26)
    %242 = acir.current {aspect = "value", resource = "top.rob.slots.0.state$value$3"} : i32 loc(#loc26)
    %243 = acir.current {aspect = "value", resource = "top.rob.slots.0.state$value$4"} : i32 loc(#loc26)
    %244 = arith.xori %239, %true : i1 loc(#loc26)
    %245 = arith.cmpi ne, %145, %c0_i32 : i32 loc(#loc27)
    %246 = arith.extui %145 : i32 to i64 loc(#loc28)
    %247 = arith.cmpi ult, %246, %c8_i64 : i64 loc(#loc28)
    %248 = arith.xori %54, %true : i1 loc(#loc29)
    %249 = arith.xori %33, %true : i1 loc(#loc29)
    %250 = arith.select %33, %248, %249 : i1 loc(#loc29)
    %251 = arith.andi %247, %250 : i1 loc(#loc29)
    %252 = arith.select %245, %251, %250 : i1 loc(#loc29)
    %253 = arith.andi %244, %252 : i1 loc(#loc29)
    %254 = arith.cmpi eq, %237, %c1_i32 : i32 loc(#loc30)
    %255 = acir.current {aspect = "value", resource = "top.rob.slots.1.state$value$0"} : i1 loc(#loc26)
    %256 = acir.current {aspect = "value", resource = "top.rob.slots.1.state$value$1"} : i1 loc(#loc26)
    %257 = acir.current {aspect = "value", resource = "top.rob.slots.1.state$value$2"} : i32 loc(#loc26)
    %258 = acir.current {aspect = "value", resource = "top.rob.slots.1.state$value$3"} : i32 loc(#loc26)
    %259 = acir.current {aspect = "value", resource = "top.rob.slots.1.state$value$4"} : i32 loc(#loc26)
    %260 = arith.xori %255, %true : i1 loc(#loc26)
    %261 = arith.andi %260, %252 : i1 loc(#loc26)
    %262 = arith.cmpi eq, %237, %c2_i32 : i32 loc(#loc31)
    %263 = acir.current {aspect = "value", resource = "top.rob.slots.2.state$value$0"} : i1 loc(#loc26)
    %264 = acir.current {aspect = "value", resource = "top.rob.slots.2.state$value$1"} : i1 loc(#loc26)
    %265 = acir.current {aspect = "value", resource = "top.rob.slots.2.state$value$2"} : i32 loc(#loc26)
    %266 = acir.current {aspect = "value", resource = "top.rob.slots.2.state$value$3"} : i32 loc(#loc26)
    %267 = acir.current {aspect = "value", resource = "top.rob.slots.2.state$value$4"} : i32 loc(#loc26)
    %268 = arith.xori %263, %true : i1 loc(#loc26)
    %269 = arith.andi %268, %252 : i1 loc(#loc26)
    %270 = arith.cmpi eq, %237, %c3_i32 : i32 loc(#loc32)
    %271 = acir.current {aspect = "value", resource = "top.rob.slots.3.state$value$0"} : i1 loc(#loc26)
    %272 = acir.current {aspect = "value", resource = "top.rob.slots.3.state$value$1"} : i1 loc(#loc26)
    %273 = acir.current {aspect = "value", resource = "top.rob.slots.3.state$value$2"} : i32 loc(#loc26)
    %274 = acir.current {aspect = "value", resource = "top.rob.slots.3.state$value$3"} : i32 loc(#loc26)
    %275 = acir.current {aspect = "value", resource = "top.rob.slots.3.state$value$4"} : i32 loc(#loc26)
    %276 = arith.xori %271, %true : i1 loc(#loc26)
    %277 = arith.andi %276, %252 : i1 loc(#loc26)
    %278 = arith.andi %270, %277 : i1 loc(#loc26)
    %279 = arith.select %262, %269, %278 : i1 loc(#loc26)
    %280 = arith.select %254, %261, %279 : i1 loc(#loc26)
    %281 = arith.select %238, %253, %280 : i1 loc(#loc26)
    %282 = arith.andi %235, %281 : i1 loc(#loc26)
    %283 = arith.subi %222, %c1_i32 : i32 loc(#loc33)
    %284 = arith.andi %283, %c3_i32 : i32 loc(#loc33)
    %285 = arith.cmpi eq, %284, %c0_i32 : i32 loc(#loc33)
    %286 = arith.cmpi eq, %284, %c1_i32 : i32 loc(#loc34)
    %287 = arith.cmpi eq, %284, %c2_i32 : i32 loc(#loc35)
    %288 = arith.cmpi eq, %284, %c3_i32 : i32 loc(#loc36)
    %289 = arith.andi %288, %282 : i1 loc(#loc36)
    %290 = arith.select %287, %282, %289 : i1 loc(#loc36)
    %291 = arith.select %286, %282, %290 : i1 loc(#loc36)
    %292 = arith.select %285, %282, %291 : i1 loc(#loc36)
    %293 = arith.select %231, %282, %292 : i1 loc(#loc36)
    %294 = arith.andi %206, %293 : i1 loc(#loc36)
    %295 = arith.select %204, %294, %282 : i1 loc(#loc36)
    %296 = arith.subi %186, %c1_i32 : i32 loc(#loc33)
    %297 = arith.andi %296, %c3_i32 : i32 loc(#loc33)
    %298 = arith.cmpi eq, %297, %c0_i32 : i32 loc(#loc33)
    %299 = arith.cmpi eq, %297, %c1_i32 : i32 loc(#loc34)
    %300 = arith.cmpi eq, %297, %c2_i32 : i32 loc(#loc35)
    %301 = arith.cmpi eq, %297, %c3_i32 : i32 loc(#loc36)
    %302 = arith.andi %301, %295 : i1 loc(#loc36)
    %303 = arith.select %300, %295, %302 : i1 loc(#loc36)
    %304 = arith.select %299, %295, %303 : i1 loc(#loc36)
    %305 = arith.select %298, %295, %304 : i1 loc(#loc36)
    %306 = arith.select %203, %295, %305 : i1 loc(#loc36)
    %307 = arith.andi %162, %306 : i1 loc(#loc36)
    %308 = arith.select %160, %307, %295 : i1 loc(#loc36)
    %309 = arith.andi %143, %308 : i1 loc(#loc36)
    %310 = arith.andi %141, %309 : i1 loc(#loc36)
    %311 = arith.andi %1, %310 : i1 loc(#loc36)
    %312 = acir.fire "top.dispatch.dispatch#6" %311 {successors = [], txn = 6 : i64} loc(#loc36)
    %313 = arith.subi %233, %c1_i32 : i32
    %314 = arith.andi %313, %c3_i32 : i32
    %315 = arith.cmpi eq, %314, %c0_i32 : i32
    %316 = arith.cmpi eq, %241, %233 : i32 loc(#loc37)
    %317 = arith.cmpi eq, %314, %c1_i32 : i32 loc(#loc38)
    %318 = arith.cmpi eq, %314, %c2_i32 : i32 loc(#loc39)
    %319 = arith.cmpi eq, %314, %c3_i32 : i32 loc(#loc40)
    %320 = arith.select %319, %273, %c0_i32 : i32
    %321 = arith.select %319, %274, %c0_i32 : i32
    %322 = arith.select %319, %275, %c0_i32 : i32
    %323 = arith.select %318, %265, %320 : i32
    %324 = arith.select %318, %266, %321 : i32
    %325 = arith.select %318, %267, %322 : i32
    %326 = arith.select %317, %257, %323 : i32
    %327 = arith.select %317, %258, %324 : i32
    %328 = arith.select %317, %259, %325 : i32
    %329 = arith.select %315, %241, %326 : i32
    %330 = arith.select %315, %242, %327 : i32
    %331 = arith.select %315, %243, %328 : i32
    %332 = arith.cmpi eq, %330, %c0_i32 : i32
    %333 = arith.extui %330 : i32 to i64 loc(#loc41)
    %334 = arith.cmpi ult, %333, %c8_i64 : i64 loc(#loc41)
    %335 = arith.cmpi eq, %257, %233 : i32 loc(#loc37)
    %336 = arith.cmpi eq, %265, %233 : i32 loc(#loc37)
    %337 = arith.cmpi eq, %273, %233 : i32 loc(#loc37)
    %338 = arith.ori %332, %334 : i1 loc(#loc37)
    %339 = arith.andi %316, %338 : i1 loc(#loc37)
    %340 = arith.andi %240, %339 : i1 loc(#loc37)
    %341 = arith.andi %239, %340 : i1 loc(#loc37)
    %342 = arith.andi %256, %335 : i1 loc(#loc37)
    %343 = arith.andi %255, %342 : i1 loc(#loc37)
    %344 = arith.andi %264, %336 : i1 loc(#loc37)
    %345 = arith.andi %263, %344 : i1 loc(#loc37)
    %346 = arith.andi %272, %337 : i1 loc(#loc37)
    %347 = arith.andi %271, %346 : i1 loc(#loc37)
    %348 = arith.andi %319, %347 : i1 loc(#loc37)
    %349 = arith.select %318, %345, %348 : i1 loc(#loc37)
    %350 = arith.select %317, %343, %349 : i1 loc(#loc37)
    %351 = arith.andi %334, %350 : i1 loc(#loc37)
    %352 = arith.select %332, %350, %351 : i1 loc(#loc37)
    %353 = arith.select %315, %341, %352 : i1 loc(#loc37)
    %354 = acir.fire "top.commit.commit#7" %353 {successors = [], txn = 7 : i64} loc(#loc37)
    %355 = arith.ori %26, %3 : i1
    %356 = arith.andi %23, %355 : i1
    %357 = acir.fire "top.audit.observe#8" %356 {successors = [], txn = 8 : i64}
    %358 = arith.andi %32, %25 : i1
    %359 = arith.cmpi eq, %28, %c0_i32 : i32
    %360 = arith.select %359, %7, %c0_i32 : i32
    %361 = arith.select %359, %8, %c0_i32 : i32
    %362 = arith.select %359, %9, %c0_i32 : i32
    %363 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$0"} : i1
    %364 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$1"} : i32
    %365 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$2"} : i32
    %366 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$3"} : i32
    %367 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$4$0"} : i1
    %368 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$4$1"} : i32
    %369 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$4$2"} : i32
    %370 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$5$0"} : i1
    %371 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$5$1"} : i32
    %372 = acir.current {aspect = "value", resource = "top.add_requests$storage$0$5$2"} : i32
    %373 = arith.addi %369, %372 : i32 loc(#loc14)
    %374 = arith.select %358, %29, %c0_i32 : i32 loc(#loc14)
    %375 = arith.cmpi eq, %374, %c0_i32 : i32 loc(#loc14)
    %376 = arith.select %375, %365, %360 : i32 loc(#loc14)
    %377 = arith.select %375, %366, %361 : i32 loc(#loc14)
    %378 = arith.select %375, %373, %362 : i32 loc(#loc14)
    %379 = arith.select %358, %376, %360 : i32 loc(#loc14)
    %380 = arith.select %358, %377, %361 : i32 loc(#loc14)
    %381 = arith.select %358, %378, %362 : i32 loc(#loc14)
    acir.next %379 {aspect = "value", resource = "top.add_done$storage$0$0"} : i32 loc(#loc14)
    acir.next %380 {aspect = "value", resource = "top.add_done$storage$0$1"} : i32 loc(#loc14)
    acir.next %381 {aspect = "value", resource = "top.add_done$storage$0$2"} : i32 loc(#loc14)
    %382 = arith.extui %358 : i1 to i32 loc(#loc14)
    %383 = arith.addi %29, %382 : i32 loc(#loc14)
    acir.next %383 {aspect = "value", resource = "top.add_done$count"} : i32 loc(#loc14)
    %384 = arith.andi %37, %41 : i1
    %385 = arith.andi %78, %384 : i1
    %386 = arith.andi %33, %385 : i1
    %387 = arith.select %77, %386, %89 : i1
    %388 = arith.select %74, %387, %386 : i1
    %389 = arith.select %72, %388, %386 : i1
    %390 = arith.select %70, %389, %386 : i1
    %391 = arith.select %68, %390, %386 : i1
    %392 = arith.select %53, %391, %89 : i1
    %393 = arith.select %51, %392, %89 : i1
    %394 = arith.select %49, %393, %89 : i1
    %395 = arith.select %47, %394, %89 : i1
    %396 = arith.andi %100, %395 : i1
    %397 = arith.cmpi eq, %79, %c0_i32 : i32 loc(#loc14)
    %398 = arith.select %397, %363, %false : i1 loc(#loc14)
    %399 = arith.select %397, %364, %c0_i32 : i32 loc(#loc14)
    %400 = arith.select %397, %365, %c0_i32 : i32 loc(#loc14)
    %401 = arith.select %397, %366, %c0_i32 : i32 loc(#loc14)
    %402 = arith.select %397, %367, %false : i1 loc(#loc14)
    %403 = arith.select %397, %368, %c0_i32 : i32 loc(#loc14)
    %404 = arith.select %397, %369, %c0_i32 : i32 loc(#loc14)
    %405 = arith.select %397, %370, %false : i1 loc(#loc14)
    %406 = arith.select %397, %371, %c0_i32 : i32 loc(#loc14)
    %407 = arith.select %397, %372, %c0_i32 : i32 loc(#loc14)
    %408 = arith.andi %47, %49 : i1
    %409 = arith.andi %408, %51 : i1
    %410 = arith.andi %409, %53 : i1
    %411 = arith.andi %68, %70 : i1
    %412 = arith.andi %411, %72 : i1
    %413 = arith.andi %412, %74 : i1
    %414 = arith.xori %413, %true : i1
    %415 = arith.ori %414, %77 : i1
    %416 = arith.andi %410, %415 : i1
    %417 = arith.select %416, %33, %54 : i1
    %418 = arith.select %416, %34, %55 : i32
    %419 = arith.select %416, %35, %56 : i32
    %420 = arith.select %416, %36, %57 : i32
    %421 = arith.select %416, %37, %58 : i1
    %422 = arith.select %416, %38, %59 : i32
    %423 = arith.select %416, %39, %60 : i32
    %424 = arith.select %416, %41, %62 : i1
    %425 = arith.select %416, %42, %63 : i32
    %426 = arith.select %416, %43, %64 : i32
    %427 = arith.select %396, %80, %c0_i32 : i32
    %428 = arith.cmpi eq, %427, %c0_i32 : i32
    %429 = arith.select %428, %417, %398 : i1
    %430 = arith.select %428, %418, %399 : i32
    %431 = arith.select %428, %419, %400 : i32
    %432 = arith.select %428, %420, %401 : i32
    %433 = arith.select %428, %421, %402 : i1
    %434 = arith.select %428, %422, %403 : i32
    %435 = arith.select %428, %423, %404 : i32
    %436 = arith.select %428, %424, %405 : i1
    %437 = arith.select %428, %425, %406 : i32
    %438 = arith.select %428, %426, %407 : i32
    %439 = arith.select %396, %429, %398 : i1
    %440 = arith.select %396, %430, %399 : i32
    %441 = arith.select %396, %431, %400 : i32
    %442 = arith.select %396, %432, %401 : i32
    %443 = arith.select %396, %433, %402 : i1
    %444 = arith.select %396, %434, %403 : i32
    %445 = arith.select %396, %435, %404 : i32
    %446 = arith.select %396, %436, %405 : i1
    %447 = arith.select %396, %437, %406 : i32
    %448 = arith.select %396, %438, %407 : i32
    acir.next %439 {aspect = "value", resource = "top.add_requests$storage$0$0"} : i1
    acir.next %440 {aspect = "value", resource = "top.add_requests$storage$0$1"} : i32
    acir.next %441 {aspect = "value", resource = "top.add_requests$storage$0$2"} : i32
    acir.next %442 {aspect = "value", resource = "top.add_requests$storage$0$3"} : i32
    acir.next %443 {aspect = "value", resource = "top.add_requests$storage$0$4$0"} : i1
    acir.next %444 {aspect = "value", resource = "top.add_requests$storage$0$4$1"} : i32
    acir.next %445 {aspect = "value", resource = "top.add_requests$storage$0$4$2"} : i32
    acir.next %446 {aspect = "value", resource = "top.add_requests$storage$0$5$0"} : i1
    acir.next %447 {aspect = "value", resource = "top.add_requests$storage$0$5$1"} : i32
    acir.next %448 {aspect = "value", resource = "top.add_requests$storage$0$5$2"} : i32
    %449 = arith.extui %396 : i1 to i32
    %450 = arith.addi %80, %449 : i32
    acir.next %450 {aspect = "value", resource = "top.add_requests$count"} : i32
    %451 = arith.select %356, %10, %c0_i32 : i32
    %452 = acir.current {aspect = "value", resource = "top.audit.last_tag$value"} : i32
    %453 = arith.select %357, %451, %452 : i32
    acir.next %453 {aspect = "value", resource = "top.audit.last_tag$value"} : i32
    %454 = acir.current {aspect = "value", resource = "top.commit.last_tag$value"} : i32 loc(#loc42)
    %455 = arith.select %354, %329, %454 : i32 loc(#loc42)
    acir.next %455 {aspect = "value", resource = "top.commit.last_tag$value"} : i32 loc(#loc42)
    %456 = acir.current {aspect = "value", resource = "top.commit.retired$value"} : i32 loc(#loc43)
    %457 = arith.addi %456, %c1_i32 : i32 loc(#loc43)
    %458 = arith.select %354, %457, %456 : i32 loc(#loc43)
    acir.next %458 {aspect = "value", resource = "top.commit.retired$value"} : i32 loc(#loc43)
    %459 = arith.andi %141, %249 : i1
    %460 = arith.andi %1, %459 : i1
    %461 = arith.andi %312, %460 : i1
    %462 = arith.cmpi ne, %186, %202 : i32 loc(#loc44)
    %463 = arith.select %301, %271, %false : i1 loc(#loc45)
    %464 = arith.select %301, %272, %false : i1 loc(#loc45)
    %465 = arith.select %301, %273, %c0_i32 : i32 loc(#loc45)
    %466 = arith.select %301, %275, %c0_i32 : i32 loc(#loc45)
    %467 = arith.select %300, %263, %463 : i1 loc(#loc45)
    %468 = arith.select %300, %264, %464 : i1 loc(#loc45)
    %469 = arith.select %300, %265, %465 : i32 loc(#loc45)
    %470 = arith.select %300, %267, %466 : i32 loc(#loc45)
    %471 = arith.select %299, %255, %467 : i1 loc(#loc45)
    %472 = arith.select %299, %256, %468 : i1 loc(#loc45)
    %473 = arith.select %299, %257, %469 : i32 loc(#loc45)
    %474 = arith.select %299, %259, %470 : i32 loc(#loc45)
    %475 = arith.select %298, %239, %471 : i1 loc(#loc45)
    %476 = arith.select %298, %240, %472 : i1 loc(#loc45)
    %477 = arith.select %298, %241, %473 : i32 loc(#loc45)
    %478 = arith.select %298, %243, %474 : i32 loc(#loc45)
    %479 = arith.andi %475, %476 : i1 loc(#loc46)
    %480 = arith.cmpi eq, %477, %186 : i32 loc(#loc47)
    %481 = arith.andi %479, %480 : i1 loc(#loc47)
    %482 = arith.select %481, %478, %c0_i32 : i32
    %483 = acir.current {aspect = "value", resource = "top.rf.registers.7$value"} : i32 loc(#loc48)
    %484 = acir.current {aspect = "value", resource = "top.rf.registers.6$value"} : i32 loc(#loc48)
    %485 = acir.current {aspect = "value", resource = "top.rf.registers.5$value"} : i32 loc(#loc48)
    %486 = acir.current {aspect = "value", resource = "top.rf.registers.4$value"} : i32 loc(#loc48)
    %487 = acir.current {aspect = "value", resource = "top.rf.registers.3$value"} : i32 loc(#loc48)
    %488 = acir.current {aspect = "value", resource = "top.rf.registers.2$value"} : i32 loc(#loc48)
    %489 = acir.current {aspect = "value", resource = "top.rf.registers.1$value"} : i32 loc(#loc48)
    %490 = acir.current {aspect = "value", resource = "top.rf.registers.0$value"} : i32 loc(#loc48)
    %491 = arith.select %177, %490, %c0_i32 : i32 loc(#loc48)
    %492 = arith.select %175, %489, %491 : i32 loc(#loc48)
    %493 = arith.select %173, %488, %492 : i32 loc(#loc48)
    %494 = arith.select %171, %487, %493 : i32 loc(#loc48)
    %495 = arith.select %169, %486, %494 : i32 loc(#loc48)
    %496 = arith.select %167, %485, %495 : i32 loc(#loc48)
    %497 = arith.select %165, %484, %496 : i32 loc(#loc48)
    %498 = arith.select %163, %483, %497 : i32 loc(#loc48)
    %499 = arith.cmpi eq, %146, %c0_i32 : i32 loc(#loc20)
    %500 = arith.select %203, %true, %499 : i1 loc(#loc49)
    %501 = arith.select %203, %186, %c0_i32 : i32 loc(#loc49)
    %502 = arith.select %203, %498, %c0_i32 : i32 loc(#loc49)
    %503 = arith.select %462, %481, %500 : i1 loc(#loc49)
    %504 = arith.select %462, %186, %501 : i32 loc(#loc49)
    %505 = arith.select %462, %482, %502 : i32 loc(#loc49)
    %506 = arith.select %160, %503, %499 : i1 loc(#loc49)
    %507 = arith.select %160, %504, %c0_i32 : i32 loc(#loc49)
    %508 = arith.select %160, %505, %c0_i32 : i32 loc(#loc49)
    %509 = arith.cmpi ne, %222, %230 : i32 loc(#loc44)
    %510 = arith.select %288, %271, %false : i1 loc(#loc45)
    %511 = arith.select %288, %272, %false : i1 loc(#loc45)
    %512 = arith.select %288, %273, %c0_i32 : i32 loc(#loc45)
    %513 = arith.select %288, %275, %c0_i32 : i32 loc(#loc45)
    %514 = arith.select %287, %263, %510 : i1 loc(#loc45)
    %515 = arith.select %287, %264, %511 : i1 loc(#loc45)
    %516 = arith.select %287, %265, %512 : i32 loc(#loc45)
    %517 = arith.select %287, %267, %513 : i32 loc(#loc45)
    %518 = arith.select %286, %255, %514 : i1 loc(#loc45)
    %519 = arith.select %286, %256, %515 : i1 loc(#loc45)
    %520 = arith.select %286, %257, %516 : i32 loc(#loc45)
    %521 = arith.select %286, %259, %517 : i32 loc(#loc45)
    %522 = arith.select %285, %239, %518 : i1 loc(#loc45)
    %523 = arith.select %285, %240, %519 : i1 loc(#loc45)
    %524 = arith.select %285, %241, %520 : i32 loc(#loc45)
    %525 = arith.select %285, %243, %521 : i32 loc(#loc45)
    %526 = arith.andi %522, %523 : i1 loc(#loc46)
    %527 = arith.cmpi eq, %524, %222 : i32 loc(#loc47)
    %528 = arith.andi %526, %527 : i1 loc(#loc47)
    %529 = arith.select %528, %525, %c0_i32 : i32 loc(#loc50)
    %530 = arith.select %214, %490, %c0_i32 : i32 loc(#loc48)
    %531 = arith.select %213, %489, %530 : i32 loc(#loc48)
    %532 = arith.select %212, %488, %531 : i32 loc(#loc48)
    %533 = arith.select %211, %487, %532 : i32 loc(#loc48)
    %534 = arith.select %210, %486, %533 : i32 loc(#loc48)
    %535 = arith.select %209, %485, %534 : i32 loc(#loc48)
    %536 = arith.select %208, %484, %535 : i32 loc(#loc48)
    %537 = arith.select %207, %483, %536 : i32 loc(#loc48)
    %538 = arith.cmpi eq, %147, %c0_i32 : i32 loc(#loc20)
    %539 = arith.select %231, %true, %538 : i1 loc(#loc20)
    %540 = arith.select %231, %222, %c0_i32 : i32 loc(#loc20)
    %541 = arith.select %231, %537, %c0_i32 : i32 loc(#loc20)
    %542 = arith.select %509, %528, %539 : i1 loc(#loc20)
    %543 = arith.select %509, %222, %540 : i32 loc(#loc20)
    %544 = arith.select %509, %529, %541 : i32 loc(#loc20)
    %545 = arith.select %204, %542, %538 : i1 loc(#loc20)
    %546 = arith.select %204, %543, %c0_i32 : i32 loc(#loc20)
    %547 = arith.select %204, %544, %c0_i32 : i32 loc(#loc20)
    %548 = arith.xori %68, %true : i1
    %549 = arith.xori %70, %true : i1
    %550 = arith.xori %72, %true : i1
    %551 = arith.cmpi ne, %73, %c0_i32 : i32 loc(#loc10)
    %552 = arith.ori %551, %77 : i1 loc(#loc10)
    %553 = arith.ori %550, %552 : i1 loc(#loc10)
    %554 = arith.ori %549, %553 : i1 loc(#loc10)
    %555 = arith.ori %548, %554 : i1 loc(#loc10)
    %556 = arith.andi %53, %555 : i1 loc(#loc10)
    %557 = arith.andi %51, %556 : i1 loc(#loc10)
    %558 = arith.andi %49, %557 : i1 loc(#loc10)
    %559 = arith.andi %47, %558 : i1 loc(#loc10)
    %560 = arith.andi %100, %559 : i1 loc(#loc10)
    %561 = arith.xori %37, %true : i1 loc(#loc51)
    %562 = arith.cmpi eq, %38, %10 : i32 loc(#loc52)
    %563 = arith.andi %561, %562 : i1 loc(#loc52)
    %564 = arith.select %563, %true, %37 : i1 loc(#loc53)
    %565 = arith.select %563, %10, %38 : i32 loc(#loc53)
    %566 = arith.select %563, %11, %39 : i32 loc(#loc53)
    %567 = arith.cmpi eq, %42, %10 : i32 loc(#loc54)
    %568 = arith.ori %567, %563 : i1 loc(#loc54)
    %569 = arith.select %41, %563, %568 : i1 loc(#loc54)
    %570 = arith.andi %33, %569 : i1 loc(#loc54)
    %571 = arith.andi %23, %570 : i1 loc(#loc54)
    %572 = arith.xori %41, %true : i1
    %573 = arith.andi %572, %567 : i1
    %574 = arith.select %573, %true, %41 : i1 loc(#loc55)
    %575 = arith.select %573, %10, %42 : i32 loc(#loc55)
    %576 = arith.select %573, %11, %43 : i32 loc(#loc55)
    %577 = arith.select %33, %564, %37 : i1 loc(#loc55)
    %578 = arith.select %33, %565, %38 : i32 loc(#loc55)
    %579 = arith.select %33, %566, %39 : i32 loc(#loc55)
    %580 = arith.select %33, %574, %41 : i1 loc(#loc55)
    %581 = arith.select %33, %575, %42 : i32 loc(#loc55)
    %582 = arith.select %33, %576, %43 : i32 loc(#loc55)
    %583 = arith.cmpi ne, %73, %c1_i32 : i32 loc(#loc18)
    %584 = arith.ori %583, %77 : i1 loc(#loc18)
    %585 = arith.ori %550, %584 : i1 loc(#loc18)
    %586 = arith.ori %549, %585 : i1 loc(#loc18)
    %587 = arith.ori %548, %586 : i1 loc(#loc18)
    %588 = arith.andi %107, %587 : i1 loc(#loc18)
    %589 = arith.andi %51, %588 : i1 loc(#loc18)
    %590 = arith.andi %49, %589 : i1 loc(#loc18)
    %591 = arith.andi %47, %590 : i1 loc(#loc18)
    %592 = arith.andi %131, %591 : i1 loc(#loc18)
    %593 = arith.select %592, %false, %33 : i1 loc(#loc18)
    %594 = arith.select %592, %c0_i32, %34 : i32 loc(#loc18)
    %595 = arith.select %592, %c0_i32, %35 : i32 loc(#loc18)
    %596 = arith.select %592, %c0_i32, %36 : i32 loc(#loc18)
    %597 = arith.select %592, %false, %37 : i1 loc(#loc18)
    %598 = arith.select %592, %c0_i32, %38 : i32 loc(#loc18)
    %599 = arith.select %592, %c0_i32, %39 : i32 loc(#loc18)
    %600 = arith.select %592, %false, %41 : i1 loc(#loc18)
    %601 = arith.select %592, %c0_i32, %42 : i32 loc(#loc18)
    %602 = arith.select %592, %c0_i32, %43 : i32 loc(#loc18)
    %603 = arith.select %571, %33, %593 : i1 loc(#loc18)
    %604 = arith.select %571, %34, %594 : i32 loc(#loc18)
    %605 = arith.select %571, %35, %595 : i32 loc(#loc18)
    %606 = arith.select %571, %36, %596 : i32 loc(#loc18)
    %607 = arith.select %571, %577, %597 : i1 loc(#loc18)
    %608 = arith.select %571, %578, %598 : i32 loc(#loc18)
    %609 = arith.select %571, %579, %599 : i32 loc(#loc18)
    %610 = arith.select %571, %580, %600 : i1 loc(#loc18)
    %611 = arith.select %571, %581, %601 : i32 loc(#loc18)
    %612 = arith.select %571, %582, %602 : i32 loc(#loc18)
    %613 = arith.select %560, %false, %603 : i1 loc(#loc18)
    %614 = arith.select %560, %c0_i32, %604 : i32 loc(#loc18)
    %615 = arith.select %560, %c0_i32, %605 : i32 loc(#loc18)
    %616 = arith.select %560, %c0_i32, %606 : i32 loc(#loc18)
    %617 = arith.select %560, %false, %607 : i1 loc(#loc18)
    %618 = arith.select %560, %c0_i32, %608 : i32 loc(#loc18)
    %619 = arith.select %560, %c0_i32, %609 : i32 loc(#loc18)
    %620 = arith.select %560, %false, %610 : i1 loc(#loc18)
    %621 = arith.select %560, %c0_i32, %611 : i32 loc(#loc18)
    %622 = arith.select %560, %c0_i32, %612 : i32 loc(#loc18)
    %623 = arith.select %461, %true, %613 : i1 loc(#loc18)
    %624 = arith.select %461, %144, %614 : i32 loc(#loc18)
    %625 = arith.select %461, %232, %615 : i32 loc(#loc18)
    %626 = arith.select %461, %145, %616 : i32 loc(#loc18)
    %627 = arith.select %461, %506, %617 : i1 loc(#loc18)
    %628 = arith.select %461, %507, %618 : i32 loc(#loc18)
    %629 = arith.select %461, %508, %619 : i32 loc(#loc18)
    %630 = arith.select %461, %545, %620 : i1 loc(#loc18)
    %631 = arith.select %461, %546, %621 : i32 loc(#loc18)
    %632 = arith.select %461, %547, %622 : i32 loc(#loc18)
    acir.next %623 {aspect = "value", resource = "top.left.slot$value$0"} : i1 loc(#loc18)
    acir.next %624 {aspect = "value", resource = "top.left.slot$value$1"} : i32 loc(#loc18)
    acir.next %625 {aspect = "value", resource = "top.left.slot$value$2"} : i32 loc(#loc18)
    acir.next %626 {aspect = "value", resource = "top.left.slot$value$3"} : i32 loc(#loc18)
    acir.next %627 {aspect = "value", resource = "top.left.slot$value$4$0"} : i1 loc(#loc18)
    acir.next %628 {aspect = "value", resource = "top.left.slot$value$4$1"} : i32 loc(#loc18)
    acir.next %629 {aspect = "value", resource = "top.left.slot$value$4$2"} : i32 loc(#loc18)
    acir.next %630 {aspect = "value", resource = "top.left.slot$value$5$0"} : i1 loc(#loc18)
    acir.next %631 {aspect = "value", resource = "top.left.slot$value$5$1"} : i32 loc(#loc18)
    acir.next %632 {aspect = "value", resource = "top.left.slot$value$5$2"} : i32 loc(#loc18)
    %633 = arith.cmpi eq, %132, %c0_i32 : i32 loc(#loc19)
    %634 = arith.andi %101, %633 : i1 loc(#loc19)
    %635 = arith.andi %140, %634 : i1 loc(#loc19)
    %636 = arith.cmpi eq, %135, %c0_i32 : i32 loc(#loc20)
    %637 = arith.select %636, %4, %c0_i32 : i32 loc(#loc20)
    %638 = arith.select %636, %5, %c0_i32 : i32 loc(#loc20)
    %639 = arith.select %636, %6, %c0_i32 : i32 loc(#loc20)
    %640 = acir.current {aspect = "value", resource = "top.multiplier.request$value$0"} : i1
    %641 = acir.current {aspect = "value", resource = "top.multiplier.request$value$1"} : i32
    %642 = acir.current {aspect = "value", resource = "top.multiplier.request$value$2"} : i32
    %643 = acir.current {aspect = "value", resource = "top.multiplier.request$value$3"} : i32
    %644 = acir.current {aspect = "value", resource = "top.multiplier.request$value$4$0"} : i1
    %645 = acir.current {aspect = "value", resource = "top.multiplier.request$value$4$1"} : i32
    %646 = acir.current {aspect = "value", resource = "top.multiplier.request$value$4$2"} : i32
    %647 = acir.current {aspect = "value", resource = "top.multiplier.request$value$5$0"} : i1
    %648 = acir.current {aspect = "value", resource = "top.multiplier.request$value$5$1"} : i32
    %649 = acir.current {aspect = "value", resource = "top.multiplier.request$value$5$2"} : i32
    %650 = arith.muli %646, %649 : i32 loc(#loc56)
    %651 = arith.select %633, %642, %c0_i32 : i32
    %652 = arith.select %633, %643, %c0_i32 : i32
    %653 = arith.select %633, %650, %c0_i32 : i32
    %654 = arith.select %635, %136, %c0_i32 : i32
    %655 = arith.cmpi eq, %654, %c0_i32 : i32
    %656 = arith.select %655, %651, %637 : i32
    %657 = arith.select %655, %652, %638 : i32
    %658 = arith.select %655, %653, %639 : i32
    %659 = arith.select %635, %656, %637 : i32
    %660 = arith.select %635, %657, %638 : i32
    %661 = arith.select %635, %658, %639 : i32
    acir.next %659 {aspect = "value", resource = "top.mul_done$storage$0$0"} : i32
    acir.next %660 {aspect = "value", resource = "top.mul_done$storage$0$1"} : i32
    acir.next %661 {aspect = "value", resource = "top.mul_done$storage$0$2"} : i32
    %662 = arith.extui %635 : i1 to i32
    %663 = arith.addi %136, %662 : i32
    acir.next %663 {aspect = "value", resource = "top.mul_done$count"} : i32
    %664 = arith.andi %109, %384 : i1
    %665 = arith.andi %33, %664 : i1
    %666 = arith.select %77, %665, %120 : i1
    %667 = arith.select %108, %666, %665 : i1
    %668 = arith.select %72, %667, %665 : i1
    %669 = arith.select %70, %668, %665 : i1
    %670 = arith.select %68, %669, %665 : i1
    %671 = arith.select %107, %670, %120 : i1
    %672 = arith.select %51, %671, %120 : i1
    %673 = arith.select %49, %672, %120 : i1
    %674 = arith.select %47, %673, %120 : i1
    %675 = arith.andi %131, %674 : i1
    %676 = arith.cmpi eq, %111, %c0_i32 : i32
    %677 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$0"} : i1 loc(#loc16)
    %678 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$1"} : i32 loc(#loc16)
    %679 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$2"} : i32 loc(#loc16)
    %680 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$3"} : i32 loc(#loc16)
    %681 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$4$0"} : i1 loc(#loc16)
    %682 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$4$1"} : i32 loc(#loc16)
    %683 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$4$2"} : i32 loc(#loc16)
    %684 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$5$0"} : i1 loc(#loc16)
    %685 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$5$1"} : i32 loc(#loc16)
    %686 = acir.current {aspect = "value", resource = "top.mul_requests$storage$0$5$2"} : i32 loc(#loc16)
    %687 = arith.select %676, %677, %false : i1
    %688 = arith.select %676, %678, %c0_i32 : i32
    %689 = arith.select %676, %679, %c0_i32 : i32
    %690 = arith.select %676, %680, %c0_i32 : i32
    %691 = arith.select %676, %681, %false : i1
    %692 = arith.select %676, %682, %c0_i32 : i32
    %693 = arith.select %676, %683, %c0_i32 : i32
    %694 = arith.select %676, %684, %false : i1
    %695 = arith.select %676, %685, %c0_i32 : i32
    %696 = arith.select %676, %686, %c0_i32 : i32
    %697 = arith.andi %409, %107 : i1 loc(#loc57)
    %698 = arith.andi %412, %108 : i1 loc(#loc58)
    %699 = arith.xori %698, %true : i1 loc(#loc58)
    %700 = arith.ori %699, %77 : i1 loc(#loc58)
    %701 = arith.andi %697, %700 : i1 loc(#loc58)
    %702 = arith.select %701, %33, %54 : i1
    %703 = arith.select %701, %34, %55 : i32
    %704 = arith.select %701, %35, %56 : i32
    %705 = arith.select %701, %36, %57 : i32
    %706 = arith.select %701, %37, %58 : i1
    %707 = arith.select %701, %38, %59 : i32
    %708 = arith.select %701, %39, %60 : i32
    %709 = arith.select %701, %41, %62 : i1
    %710 = arith.select %701, %42, %63 : i32
    %711 = arith.select %701, %43, %64 : i32
    %712 = arith.select %675, %112, %c0_i32 : i32
    %713 = arith.cmpi eq, %712, %c0_i32 : i32
    %714 = arith.select %713, %702, %687 : i1
    %715 = arith.select %713, %703, %688 : i32
    %716 = arith.select %713, %704, %689 : i32
    %717 = arith.select %713, %705, %690 : i32
    %718 = arith.select %713, %706, %691 : i1
    %719 = arith.select %713, %707, %692 : i32
    %720 = arith.select %713, %708, %693 : i32
    %721 = arith.select %713, %709, %694 : i1
    %722 = arith.select %713, %710, %695 : i32
    %723 = arith.select %713, %711, %696 : i32
    %724 = arith.select %675, %714, %687 : i1
    %725 = arith.select %675, %715, %688 : i32
    %726 = arith.select %675, %716, %689 : i32
    %727 = arith.select %675, %717, %690 : i32
    %728 = arith.select %675, %718, %691 : i1
    %729 = arith.select %675, %719, %692 : i32
    %730 = arith.select %675, %720, %693 : i32
    %731 = arith.select %675, %721, %694 : i1
    %732 = arith.select %675, %722, %695 : i32
    %733 = arith.select %675, %723, %696 : i32
    acir.next %724 {aspect = "value", resource = "top.mul_requests$storage$0$0"} : i1
    acir.next %725 {aspect = "value", resource = "top.mul_requests$storage$0$1"} : i32
    acir.next %726 {aspect = "value", resource = "top.mul_requests$storage$0$2"} : i32
    acir.next %727 {aspect = "value", resource = "top.mul_requests$storage$0$3"} : i32
    acir.next %728 {aspect = "value", resource = "top.mul_requests$storage$0$4$0"} : i1
    acir.next %729 {aspect = "value", resource = "top.mul_requests$storage$0$4$1"} : i32
    acir.next %730 {aspect = "value", resource = "top.mul_requests$storage$0$4$2"} : i32
    acir.next %731 {aspect = "value", resource = "top.mul_requests$storage$0$5$0"} : i1
    acir.next %732 {aspect = "value", resource = "top.mul_requests$storage$0$5$1"} : i32
    acir.next %733 {aspect = "value", resource = "top.mul_requests$storage$0$5$2"} : i32
    %734 = arith.extui %675 : i1 to i32
    %735 = arith.addi %112, %734 : i32
    acir.next %735 {aspect = "value", resource = "top.mul_requests$count"} : i32
    %736 = arith.select %110, %true, %101 : i1
    %737 = arith.select %635, %false, %736 : i1
    acir.next %737 {aspect = "value", resource = "top.multiplier.active$value"} : i1
    %738 = arith.andi %101, %133 : i1
    %739 = arith.andi %140, %738 : i1
    %740 = arith.subi %132, %c1_i32 : i32 loc(#loc59)
    %741 = arith.select %110, %c2_i32, %132 : i32
    %742 = arith.select %739, %740, %741 : i32
    acir.next %742 {aspect = "value", resource = "top.multiplier.remaining$value"} : i32
    %743 = arith.select %110, %677, %640 : i1 loc(#loc16)
    %744 = arith.select %110, %678, %641 : i32 loc(#loc16)
    %745 = arith.select %110, %679, %642 : i32 loc(#loc16)
    %746 = arith.select %110, %680, %643 : i32 loc(#loc16)
    %747 = arith.select %110, %681, %644 : i1 loc(#loc16)
    %748 = arith.select %110, %682, %645 : i32 loc(#loc16)
    %749 = arith.select %110, %683, %646 : i32 loc(#loc16)
    %750 = arith.select %110, %684, %647 : i1 loc(#loc16)
    %751 = arith.select %110, %685, %648 : i32 loc(#loc16)
    %752 = arith.select %110, %686, %649 : i32 loc(#loc16)
    acir.next %743 {aspect = "value", resource = "top.multiplier.request$value$0"} : i1 loc(#loc16)
    acir.next %744 {aspect = "value", resource = "top.multiplier.request$value$1"} : i32 loc(#loc16)
    acir.next %745 {aspect = "value", resource = "top.multiplier.request$value$2"} : i32 loc(#loc16)
    acir.next %746 {aspect = "value", resource = "top.multiplier.request$value$3"} : i32 loc(#loc16)
    acir.next %747 {aspect = "value", resource = "top.multiplier.request$value$4$0"} : i1 loc(#loc16)
    acir.next %748 {aspect = "value", resource = "top.multiplier.request$value$4$1"} : i32 loc(#loc16)
    acir.next %749 {aspect = "value", resource = "top.multiplier.request$value$4$2"} : i32 loc(#loc16)
    acir.next %750 {aspect = "value", resource = "top.multiplier.request$value$5$0"} : i1 loc(#loc16)
    acir.next %751 {aspect = "value", resource = "top.multiplier.request$value$5$1"} : i32 loc(#loc16)
    acir.next %752 {aspect = "value", resource = "top.multiplier.request$value$5$2"} : i32 loc(#loc16)
    %753 = arith.cmpi ne, %330, %c0_i32 : i32 loc(#loc60)
    %754 = arith.andi %354, %753 : i1 loc(#loc60)
    %755 = arith.cmpi eq, %333, %c0_i64 : i64 loc(#loc61)
    %756 = arith.andi %754, %755 : i1 loc(#loc61)
    %757 = arith.select %756, %329, %194 : i32 loc(#loc61)
    acir.next %757 {aspect = "value", resource = "top.rf.committed.0$value"} : i32 loc(#loc61)
    %758 = arith.cmpi eq, %333, %c1_i64 : i64 loc(#loc61)
    %759 = arith.andi %754, %758 : i1 loc(#loc61)
    %760 = arith.select %759, %329, %193 : i32 loc(#loc61)
    acir.next %760 {aspect = "value", resource = "top.rf.committed.1$value"} : i32 loc(#loc61)
    %761 = arith.cmpi eq, %333, %c2_i64 : i64 loc(#loc61)
    %762 = arith.andi %754, %761 : i1 loc(#loc61)
    %763 = arith.select %762, %329, %192 : i32 loc(#loc61)
    acir.next %763 {aspect = "value", resource = "top.rf.committed.2$value"} : i32 loc(#loc61)
    %764 = arith.cmpi eq, %333, %c3_i64 : i64 loc(#loc61)
    %765 = arith.andi %754, %764 : i1 loc(#loc61)
    %766 = arith.select %765, %329, %191 : i32 loc(#loc61)
    acir.next %766 {aspect = "value", resource = "top.rf.committed.3$value"} : i32 loc(#loc61)
    %767 = arith.cmpi eq, %333, %c4_i64 : i64 loc(#loc61)
    %768 = arith.andi %754, %767 : i1 loc(#loc61)
    %769 = arith.select %768, %329, %190 : i32 loc(#loc61)
    acir.next %769 {aspect = "value", resource = "top.rf.committed.4$value"} : i32 loc(#loc61)
    %770 = arith.cmpi eq, %333, %c5_i64 : i64 loc(#loc61)
    %771 = arith.andi %754, %770 : i1 loc(#loc61)
    %772 = arith.select %771, %329, %189 : i32 loc(#loc61)
    acir.next %772 {aspect = "value", resource = "top.rf.committed.5$value"} : i32 loc(#loc61)
    %773 = arith.cmpi eq, %333, %c6_i64 : i64 loc(#loc61)
    %774 = arith.andi %754, %773 : i1 loc(#loc61)
    %775 = arith.select %774, %329, %188 : i32 loc(#loc61)
    acir.next %775 {aspect = "value", resource = "top.rf.committed.6$value"} : i32 loc(#loc61)
    %776 = arith.cmpi eq, %333, %c7_i64 : i64 loc(#loc61)
    %777 = arith.andi %754, %776 : i1 loc(#loc61)
    %778 = arith.select %777, %329, %187 : i32 loc(#loc61)
    acir.next %778 {aspect = "value", resource = "top.rf.committed.7$value"} : i32 loc(#loc61)
    %779 = arith.select %756, %331, %490 : i32 loc(#loc62)
    acir.next %779 {aspect = "value", resource = "top.rf.registers.0$value"} : i32 loc(#loc62)
    %780 = arith.select %759, %331, %489 : i32 loc(#loc41)
    acir.next %780 {aspect = "value", resource = "top.rf.registers.1$value"} : i32 loc(#loc41)
    %781 = arith.select %762, %331, %488 : i32 loc(#loc41)
    acir.next %781 {aspect = "value", resource = "top.rf.registers.2$value"} : i32 loc(#loc41)
    %782 = arith.select %765, %331, %487 : i32 loc(#loc41)
    acir.next %782 {aspect = "value", resource = "top.rf.registers.3$value"} : i32 loc(#loc41)
    %783 = arith.select %768, %331, %486 : i32 loc(#loc41)
    acir.next %783 {aspect = "value", resource = "top.rf.registers.4$value"} : i32 loc(#loc41)
    %784 = arith.select %771, %331, %485 : i32 loc(#loc41)
    acir.next %784 {aspect = "value", resource = "top.rf.registers.5$value"} : i32 loc(#loc41)
    %785 = arith.select %774, %331, %484 : i32 loc(#loc41)
    acir.next %785 {aspect = "value", resource = "top.rf.registers.6$value"} : i32 loc(#loc41)
    %786 = arith.select %777, %331, %483 : i32 loc(#loc41)
    acir.next %786 {aspect = "value", resource = "top.rf.registers.7$value"} : i32 loc(#loc41)
    %787 = arith.andi %141, %245 : i1
    %788 = arith.andi %1, %787 : i1
    %789 = arith.andi %312, %788 : i1
    %790 = arith.cmpi eq, %246, %c0_i64 : i64 loc(#loc28)
    %791 = arith.andi %789, %790 : i1 loc(#loc28)
    %792 = arith.select %791, %232, %178 : i32 loc(#loc28)
    acir.next %792 {aspect = "value", resource = "top.rf.renamed.0$value"} : i32 loc(#loc28)
    %793 = arith.cmpi eq, %246, %c1_i64 : i64 loc(#loc28)
    %794 = arith.andi %789, %793 : i1 loc(#loc28)
    %795 = arith.select %794, %232, %176 : i32 loc(#loc28)
    acir.next %795 {aspect = "value", resource = "top.rf.renamed.1$value"} : i32 loc(#loc28)
    %796 = arith.cmpi eq, %246, %c2_i64 : i64 loc(#loc28)
    %797 = arith.andi %789, %796 : i1 loc(#loc28)
    %798 = arith.select %797, %232, %174 : i32 loc(#loc28)
    acir.next %798 {aspect = "value", resource = "top.rf.renamed.2$value"} : i32 loc(#loc28)
    %799 = arith.cmpi eq, %246, %c3_i64 : i64 loc(#loc28)
    %800 = arith.andi %789, %799 : i1 loc(#loc28)
    %801 = arith.select %800, %232, %172 : i32 loc(#loc28)
    acir.next %801 {aspect = "value", resource = "top.rf.renamed.3$value"} : i32 loc(#loc28)
    %802 = arith.cmpi eq, %246, %c4_i64 : i64 loc(#loc28)
    %803 = arith.andi %789, %802 : i1 loc(#loc28)
    %804 = arith.select %803, %232, %170 : i32 loc(#loc28)
    acir.next %804 {aspect = "value", resource = "top.rf.renamed.4$value"} : i32 loc(#loc28)
    %805 = arith.cmpi eq, %246, %c5_i64 : i64 loc(#loc28)
    %806 = arith.andi %789, %805 : i1 loc(#loc28)
    %807 = arith.select %806, %232, %168 : i32 loc(#loc28)
    acir.next %807 {aspect = "value", resource = "top.rf.renamed.5$value"} : i32 loc(#loc28)
    %808 = arith.cmpi eq, %246, %c6_i64 : i64 loc(#loc28)
    %809 = arith.andi %789, %808 : i1 loc(#loc28)
    %810 = arith.select %809, %232, %166 : i32 loc(#loc28)
    acir.next %810 {aspect = "value", resource = "top.rf.renamed.6$value"} : i32 loc(#loc28)
    %811 = arith.cmpi eq, %246, %c7_i64 : i64 loc(#loc28)
    %812 = arith.andi %789, %811 : i1 loc(#loc28)
    %813 = arith.select %812, %232, %164 : i32 loc(#loc28)
    acir.next %813 {aspect = "value", resource = "top.rf.renamed.7$value"} : i32 loc(#loc28)
    %814 = arith.andi %141, %33 : i1
    %815 = arith.andi %1, %814 : i1
    %816 = arith.andi %312, %815 : i1
    %817 = arith.xori %47, %true : i1
    %818 = arith.xori %49, %true : i1
    %819 = arith.xori %51, %true : i1
    %820 = arith.cmpi ne, %52, %c0_i32 : i32 loc(#loc7)
    %821 = arith.cmpi uge, %75, %76 : i32 loc(#loc11)
    %822 = arith.andi %74, %821 : i1 loc(#loc11)
    %823 = arith.andi %72, %822 : i1 loc(#loc11)
    %824 = arith.andi %70, %823 : i1 loc(#loc11)
    %825 = arith.andi %68, %824 : i1 loc(#loc11)
    %826 = arith.ori %820, %825 : i1 loc(#loc11)
    %827 = arith.ori %819, %826 : i1 loc(#loc11)
    %828 = arith.ori %818, %827 : i1 loc(#loc11)
    %829 = arith.ori %817, %828 : i1 loc(#loc11)
    %830 = arith.andi %100, %829 : i1 loc(#loc11)
    %831 = arith.xori %58, %true : i1 loc(#loc51)
    %832 = arith.cmpi eq, %59, %10 : i32 loc(#loc52)
    %833 = arith.andi %831, %832 : i1 loc(#loc52)
    %834 = arith.select %833, %true, %58 : i1 loc(#loc53)
    %835 = arith.select %833, %10, %59 : i32 loc(#loc53)
    %836 = arith.select %833, %11, %60 : i32 loc(#loc53)
    %837 = arith.cmpi eq, %63, %10 : i32 loc(#loc54)
    %838 = arith.ori %837, %833 : i1 loc(#loc54)
    %839 = arith.select %62, %833, %838 : i1 loc(#loc54)
    %840 = arith.andi %54, %839 : i1 loc(#loc54)
    %841 = arith.andi %23, %840 : i1 loc(#loc54)
    %842 = arith.xori %62, %true : i1
    %843 = arith.andi %842, %837 : i1
    %844 = arith.select %843, %true, %62 : i1 loc(#loc55)
    %845 = arith.select %843, %10, %63 : i32 loc(#loc55)
    %846 = arith.select %843, %11, %64 : i32 loc(#loc55)
    %847 = arith.select %54, %834, %58 : i1 loc(#loc55)
    %848 = arith.select %54, %835, %59 : i32 loc(#loc55)
    %849 = arith.select %54, %836, %60 : i32 loc(#loc55)
    %850 = arith.select %54, %844, %62 : i1 loc(#loc55)
    %851 = arith.select %54, %845, %63 : i32 loc(#loc55)
    %852 = arith.select %54, %846, %64 : i32 loc(#loc55)
    %853 = arith.cmpi ne, %52, %c1_i32 : i32 loc(#loc17)
    %854 = arith.andi %108, %821 : i1 loc(#loc11)
    %855 = arith.andi %72, %854 : i1 loc(#loc11)
    %856 = arith.andi %70, %855 : i1 loc(#loc11)
    %857 = arith.andi %68, %856 : i1 loc(#loc11)
    %858 = arith.ori %853, %857 : i1 loc(#loc11)
    %859 = arith.ori %819, %858 : i1 loc(#loc11)
    %860 = arith.ori %818, %859 : i1 loc(#loc11)
    %861 = arith.ori %817, %860 : i1 loc(#loc11)
    %862 = arith.andi %131, %861 : i1 loc(#loc11)
    %863 = arith.select %862, %false, %54 : i1 loc(#loc11)
    %864 = arith.select %862, %c0_i32, %55 : i32 loc(#loc11)
    %865 = arith.select %862, %c0_i32, %56 : i32 loc(#loc11)
    %866 = arith.select %862, %c0_i32, %57 : i32 loc(#loc11)
    %867 = arith.select %862, %false, %58 : i1 loc(#loc11)
    %868 = arith.select %862, %c0_i32, %59 : i32 loc(#loc11)
    %869 = arith.select %862, %c0_i32, %60 : i32 loc(#loc11)
    %870 = arith.select %862, %false, %62 : i1 loc(#loc11)
    %871 = arith.select %862, %c0_i32, %63 : i32 loc(#loc11)
    %872 = arith.select %862, %c0_i32, %64 : i32 loc(#loc11)
    %873 = arith.select %841, %54, %863 : i1 loc(#loc11)
    %874 = arith.select %841, %55, %864 : i32 loc(#loc11)
    %875 = arith.select %841, %56, %865 : i32 loc(#loc11)
    %876 = arith.select %841, %57, %866 : i32 loc(#loc11)
    %877 = arith.select %841, %847, %867 : i1 loc(#loc11)
    %878 = arith.select %841, %848, %868 : i32 loc(#loc11)
    %879 = arith.select %841, %849, %869 : i32 loc(#loc11)
    %880 = arith.select %841, %850, %870 : i1 loc(#loc11)
    %881 = arith.select %841, %851, %871 : i32 loc(#loc11)
    %882 = arith.select %841, %852, %872 : i32 loc(#loc11)
    %883 = arith.select %830, %false, %873 : i1 loc(#loc11)
    %884 = arith.select %830, %c0_i32, %874 : i32 loc(#loc11)
    %885 = arith.select %830, %c0_i32, %875 : i32 loc(#loc11)
    %886 = arith.select %830, %c0_i32, %876 : i32 loc(#loc11)
    %887 = arith.select %830, %false, %877 : i1 loc(#loc11)
    %888 = arith.select %830, %c0_i32, %878 : i32 loc(#loc11)
    %889 = arith.select %830, %c0_i32, %879 : i32 loc(#loc11)
    %890 = arith.select %830, %false, %880 : i1 loc(#loc11)
    %891 = arith.select %830, %c0_i32, %881 : i32 loc(#loc11)
    %892 = arith.select %830, %c0_i32, %882 : i32 loc(#loc11)
    %893 = arith.select %816, %true, %883 : i1 loc(#loc11)
    %894 = arith.select %816, %144, %884 : i32 loc(#loc11)
    %895 = arith.select %816, %232, %885 : i32 loc(#loc11)
    %896 = arith.select %816, %145, %886 : i32 loc(#loc11)
    %897 = arith.select %816, %506, %887 : i1 loc(#loc11)
    %898 = arith.select %816, %507, %888 : i32 loc(#loc11)
    %899 = arith.select %816, %508, %889 : i32 loc(#loc11)
    %900 = arith.select %816, %545, %890 : i1 loc(#loc11)
    %901 = arith.select %816, %546, %891 : i32 loc(#loc11)
    %902 = arith.select %816, %547, %892 : i32 loc(#loc11)
    acir.next %893 {aspect = "value", resource = "top.right.slot$value$0"} : i1 loc(#loc11)
    acir.next %894 {aspect = "value", resource = "top.right.slot$value$1"} : i32 loc(#loc11)
    acir.next %895 {aspect = "value", resource = "top.right.slot$value$2"} : i32 loc(#loc11)
    acir.next %896 {aspect = "value", resource = "top.right.slot$value$3"} : i32 loc(#loc11)
    acir.next %897 {aspect = "value", resource = "top.right.slot$value$4$0"} : i1 loc(#loc11)
    acir.next %898 {aspect = "value", resource = "top.right.slot$value$4$1"} : i32 loc(#loc11)
    acir.next %899 {aspect = "value", resource = "top.right.slot$value$4$2"} : i32 loc(#loc11)
    acir.next %900 {aspect = "value", resource = "top.right.slot$value$5$0"} : i1 loc(#loc11)
    acir.next %901 {aspect = "value", resource = "top.right.slot$value$5$1"} : i32 loc(#loc11)
    acir.next %902 {aspect = "value", resource = "top.right.slot$value$5$2"} : i32 loc(#loc11)
    %903 = arith.addi %233, %c1_i32 : i32 loc(#loc63)
    %904 = arith.select %354, %903, %233 : i32 loc(#loc63)
    acir.next %904 {aspect = "value", resource = "top.rob.head$value"} : i32 loc(#loc63)
    %905 = arith.andi %354, %315 : i1
    %906 = arith.andi %141, %238 : i1
    %907 = arith.andi %1, %906 : i1
    %908 = arith.andi %312, %907 : i1
    %909 = arith.xori %240, %true : i1 loc(#loc64)
    %910 = arith.cmpi eq, %241, %10 : i32 loc(#loc65)
    %911 = arith.andi %909, %910 : i1 loc(#loc65)
    %912 = arith.andi %239, %911 : i1 loc(#loc65)
    %913 = arith.andi %14, %912 : i1 loc(#loc65)
    %914 = arith.andi %23, %913 : i1 loc(#loc65)
    %915 = arith.select %914, %true, %240 : i1 loc(#loc66)
    %916 = arith.select %914, %11, %243 : i32 loc(#loc66)
    %917 = arith.select %908, %true, %239 : i1 loc(#loc66)
    %918 = arith.select %908, %false, %915 : i1 loc(#loc66)
    %919 = arith.select %908, %232, %241 : i32 loc(#loc66)
    %920 = arith.select %908, %145, %242 : i32 loc(#loc66)
    %921 = arith.select %908, %c0_i32, %916 : i32 loc(#loc66)
    %922 = arith.select %905, %false, %917 : i1 loc(#loc66)
    %923 = arith.select %905, %false, %918 : i1 loc(#loc66)
    %924 = arith.select %905, %c0_i32, %919 : i32 loc(#loc66)
    %925 = arith.select %905, %c0_i32, %920 : i32 loc(#loc66)
    %926 = arith.select %905, %c0_i32, %921 : i32 loc(#loc66)
    acir.next %922 {aspect = "value", resource = "top.rob.slots.0.state$value$0"} : i1 loc(#loc66)
    acir.next %923 {aspect = "value", resource = "top.rob.slots.0.state$value$1"} : i1 loc(#loc66)
    acir.next %924 {aspect = "value", resource = "top.rob.slots.0.state$value$2"} : i32 loc(#loc66)
    acir.next %925 {aspect = "value", resource = "top.rob.slots.0.state$value$3"} : i32 loc(#loc66)
    acir.next %926 {aspect = "value", resource = "top.rob.slots.0.state$value$4"} : i32 loc(#loc66)
    %927 = arith.cmpi ne, %314, %c0_i32 : i32 loc(#loc67)
    %928 = arith.andi %927, %317 : i1 loc(#loc67)
    %929 = arith.andi %354, %928 : i1 loc(#loc67)
    %930 = arith.cmpi ne, %237, %c0_i32 : i32 loc(#loc68)
    %931 = arith.andi %930, %254 : i1 loc(#loc68)
    %932 = arith.andi %141, %931 : i1 loc(#loc68)
    %933 = arith.andi %1, %932 : i1 loc(#loc68)
    %934 = arith.andi %312, %933 : i1 loc(#loc68)
    %935 = arith.cmpi ne, %13, %c0_i32 : i32 loc(#loc69)
    %936 = arith.xori %256, %true : i1 loc(#loc64)
    %937 = arith.cmpi eq, %257, %10 : i32 loc(#loc65)
    %938 = arith.andi %936, %937 : i1 loc(#loc65)
    %939 = arith.andi %255, %938 : i1 loc(#loc65)
    %940 = arith.andi %15, %939 : i1 loc(#loc65)
    %941 = arith.andi %935, %940 : i1 loc(#loc65)
    %942 = arith.andi %23, %941 : i1 loc(#loc65)
    %943 = arith.select %942, %true, %256 : i1 loc(#loc66)
    %944 = arith.select %942, %11, %259 : i32 loc(#loc66)
    %945 = arith.select %934, %true, %255 : i1 loc(#loc66)
    %946 = arith.select %934, %false, %943 : i1 loc(#loc66)
    %947 = arith.select %934, %232, %257 : i32 loc(#loc66)
    %948 = arith.select %934, %145, %258 : i32 loc(#loc66)
    %949 = arith.select %934, %c0_i32, %944 : i32 loc(#loc66)
    %950 = arith.select %929, %false, %945 : i1 loc(#loc66)
    %951 = arith.select %929, %false, %946 : i1 loc(#loc66)
    %952 = arith.select %929, %c0_i32, %947 : i32 loc(#loc66)
    %953 = arith.select %929, %c0_i32, %948 : i32 loc(#loc66)
    %954 = arith.select %929, %c0_i32, %949 : i32 loc(#loc66)
    acir.next %950 {aspect = "value", resource = "top.rob.slots.1.state$value$0"} : i1 loc(#loc66)
    acir.next %951 {aspect = "value", resource = "top.rob.slots.1.state$value$1"} : i1 loc(#loc66)
    acir.next %952 {aspect = "value", resource = "top.rob.slots.1.state$value$2"} : i32 loc(#loc66)
    acir.next %953 {aspect = "value", resource = "top.rob.slots.1.state$value$3"} : i32 loc(#loc66)
    acir.next %954 {aspect = "value", resource = "top.rob.slots.1.state$value$4"} : i32 loc(#loc66)
    %955 = arith.cmpi ne, %314, %c1_i32 : i32 loc(#loc38)
    %956 = arith.andi %955, %318 : i1 loc(#loc38)
    %957 = arith.andi %927, %956 : i1 loc(#loc38)
    %958 = arith.andi %354, %957 : i1 loc(#loc38)
    %959 = arith.cmpi ne, %237, %c1_i32 : i32 loc(#loc30)
    %960 = arith.andi %959, %262 : i1 loc(#loc30)
    %961 = arith.andi %930, %960 : i1 loc(#loc30)
    %962 = arith.andi %141, %961 : i1 loc(#loc30)
    %963 = arith.andi %1, %962 : i1 loc(#loc30)
    %964 = arith.andi %312, %963 : i1 loc(#loc30)
    %965 = arith.cmpi ne, %13, %c1_i32 : i32 loc(#loc2)
    %966 = arith.xori %264, %true : i1 loc(#loc64)
    %967 = arith.cmpi eq, %265, %10 : i32 loc(#loc65)
    %968 = arith.andi %966, %967 : i1 loc(#loc65)
    %969 = arith.andi %263, %968 : i1 loc(#loc65)
    %970 = arith.andi %16, %969 : i1 loc(#loc65)
    %971 = arith.andi %965, %970 : i1 loc(#loc65)
    %972 = arith.andi %935, %971 : i1 loc(#loc65)
    %973 = arith.andi %23, %972 : i1 loc(#loc65)
    %974 = arith.select %973, %true, %264 : i1 loc(#loc66)
    %975 = arith.select %973, %11, %267 : i32 loc(#loc66)
    %976 = arith.select %964, %true, %263 : i1 loc(#loc66)
    %977 = arith.select %964, %false, %974 : i1 loc(#loc66)
    %978 = arith.select %964, %232, %265 : i32 loc(#loc66)
    %979 = arith.select %964, %145, %266 : i32 loc(#loc66)
    %980 = arith.select %964, %c0_i32, %975 : i32 loc(#loc66)
    %981 = arith.select %958, %false, %976 : i1 loc(#loc66)
    %982 = arith.select %958, %false, %977 : i1 loc(#loc66)
    %983 = arith.select %958, %c0_i32, %978 : i32 loc(#loc66)
    %984 = arith.select %958, %c0_i32, %979 : i32 loc(#loc66)
    %985 = arith.select %958, %c0_i32, %980 : i32 loc(#loc66)
    acir.next %981 {aspect = "value", resource = "top.rob.slots.2.state$value$0"} : i1 loc(#loc66)
    acir.next %982 {aspect = "value", resource = "top.rob.slots.2.state$value$1"} : i1 loc(#loc66)
    acir.next %983 {aspect = "value", resource = "top.rob.slots.2.state$value$2"} : i32 loc(#loc66)
    acir.next %984 {aspect = "value", resource = "top.rob.slots.2.state$value$3"} : i32 loc(#loc66)
    acir.next %985 {aspect = "value", resource = "top.rob.slots.2.state$value$4"} : i32 loc(#loc66)
    %986 = arith.cmpi ne, %314, %c2_i32 : i32 loc(#loc39)
    %987 = arith.andi %986, %319 : i1 loc(#loc39)
    %988 = arith.andi %955, %987 : i1 loc(#loc39)
    %989 = arith.andi %927, %988 : i1 loc(#loc39)
    %990 = arith.andi %354, %989 : i1 loc(#loc39)
    %991 = arith.cmpi ne, %237, %c2_i32 : i32 loc(#loc31)
    %992 = arith.andi %991, %270 : i1 loc(#loc31)
    %993 = arith.andi %959, %992 : i1 loc(#loc31)
    %994 = arith.andi %930, %993 : i1 loc(#loc31)
    %995 = arith.andi %141, %994 : i1 loc(#loc31)
    %996 = arith.andi %1, %995 : i1 loc(#loc31)
    %997 = arith.andi %312, %996 : i1 loc(#loc31)
    %998 = arith.cmpi ne, %13, %c2_i32 : i32 loc(#loc70)
    %999 = arith.xori %272, %true : i1 loc(#loc64)
    %1000 = arith.cmpi eq, %273, %10 : i32 loc(#loc65)
    %1001 = arith.andi %999, %1000 : i1 loc(#loc65)
    %1002 = arith.andi %271, %1001 : i1 loc(#loc65)
    %1003 = arith.andi %17, %1002 : i1 loc(#loc65)
    %1004 = arith.andi %998, %1003 : i1 loc(#loc65)
    %1005 = arith.andi %965, %1004 : i1 loc(#loc65)
    %1006 = arith.andi %935, %1005 : i1 loc(#loc65)
    %1007 = arith.andi %23, %1006 : i1 loc(#loc65)
    %1008 = arith.select %1007, %true, %272 : i1 loc(#loc66)
    %1009 = arith.select %1007, %11, %275 : i32 loc(#loc66)
    %1010 = arith.select %997, %true, %271 : i1 loc(#loc66)
    %1011 = arith.select %997, %false, %1008 : i1 loc(#loc66)
    %1012 = arith.select %997, %232, %273 : i32 loc(#loc66)
    %1013 = arith.select %997, %145, %274 : i32 loc(#loc66)
    %1014 = arith.select %997, %c0_i32, %1009 : i32 loc(#loc66)
    %1015 = arith.select %990, %false, %1010 : i1 loc(#loc66)
    %1016 = arith.select %990, %false, %1011 : i1 loc(#loc66)
    %1017 = arith.select %990, %c0_i32, %1012 : i32 loc(#loc66)
    %1018 = arith.select %990, %c0_i32, %1013 : i32 loc(#loc66)
    %1019 = arith.select %990, %c0_i32, %1014 : i32 loc(#loc66)
    acir.next %1015 {aspect = "value", resource = "top.rob.slots.3.state$value$0"} : i1 loc(#loc66)
    acir.next %1016 {aspect = "value", resource = "top.rob.slots.3.state$value$1"} : i1 loc(#loc66)
    acir.next %1017 {aspect = "value", resource = "top.rob.slots.3.state$value$2"} : i32 loc(#loc66)
    acir.next %1018 {aspect = "value", resource = "top.rob.slots.3.state$value$3"} : i32 loc(#loc66)
    acir.next %1019 {aspect = "value", resource = "top.rob.slots.3.state$value$4"} : i32 loc(#loc66)
    %1020 = arith.andi %1, %141 : i1
    %1021 = arith.andi %312, %1020 : i1
    %1022 = arith.addi %232, %c1_i32 : i32 loc(#loc71)
    %1023 = arith.select %1021, %1022, %232 : i32 loc(#loc71)
    acir.next %1023 {aspect = "value", resource = "top.rob.tail$value"} : i32 loc(#loc71)
    %1024 = arith.extui %1021 : i1 to i32 loc(#loc71)
    %1025 = arith.cmpi eq, %1024, %c0_i32 : i32 loc(#loc71)
    %1026 = arith.cmpi eq, %1024, %c1_i32 : i32 loc(#loc71)
    %1027 = arith.cmpi eq, %1024, %c2_i32 : i32 loc(#loc71)
    %1028 = arith.cmpi eq, %1024, %c3_i32 : i32 loc(#loc71)
    %1029 = arith.select %1028, %156, %c0_i32 : i32 loc(#loc71)
    %1030 = arith.select %1028, %157, %c0_i32 : i32 loc(#loc71)
    %1031 = arith.select %1028, %158, %c0_i32 : i32 loc(#loc71)
    %1032 = arith.select %1028, %159, %c0_i32 : i32 loc(#loc71)
    %1033 = arith.select %1027, %152, %1029 : i32 loc(#loc71)
    %1034 = arith.select %1027, %153, %1030 : i32 loc(#loc71)
    %1035 = arith.select %1027, %154, %1031 : i32 loc(#loc71)
    %1036 = arith.select %1027, %155, %1032 : i32 loc(#loc71)
    %1037 = arith.select %1026, %148, %1033 : i32 loc(#loc71)
    %1038 = arith.select %1026, %149, %1034 : i32 loc(#loc71)
    %1039 = arith.select %1026, %150, %1035 : i32 loc(#loc71)
    %1040 = arith.select %1026, %151, %1036 : i32 loc(#loc71)
    %1041 = arith.select %1025, %144, %1037 : i32 loc(#loc71)
    %1042 = arith.select %1025, %145, %1038 : i32 loc(#loc71)
    %1043 = arith.select %1025, %146, %1039 : i32 loc(#loc71)
    %1044 = arith.select %1025, %147, %1040 : i32 loc(#loc71)
    %1045 = arith.select %1027, %156, %c0_i32 : i32 loc(#loc71)
    %1046 = arith.select %1027, %157, %c0_i32 : i32 loc(#loc71)
    %1047 = arith.select %1027, %158, %c0_i32 : i32 loc(#loc71)
    %1048 = arith.select %1027, %159, %c0_i32 : i32 loc(#loc71)
    %1049 = arith.select %1026, %152, %1045 : i32 loc(#loc71)
    %1050 = arith.select %1026, %153, %1046 : i32 loc(#loc71)
    %1051 = arith.select %1026, %154, %1047 : i32 loc(#loc71)
    %1052 = arith.select %1026, %155, %1048 : i32 loc(#loc71)
    %1053 = arith.select %1025, %148, %1049 : i32 loc(#loc71)
    %1054 = arith.select %1025, %149, %1050 : i32 loc(#loc71)
    %1055 = arith.select %1025, %150, %1051 : i32 loc(#loc71)
    %1056 = arith.select %1025, %151, %1052 : i32 loc(#loc71)
    %1057 = arith.select %1026, %156, %c0_i32 : i32 loc(#loc71)
    %1058 = arith.select %1026, %157, %c0_i32 : i32 loc(#loc71)
    %1059 = arith.select %1026, %158, %c0_i32 : i32 loc(#loc71)
    %1060 = arith.select %1026, %159, %c0_i32 : i32 loc(#loc71)
    %1061 = arith.select %1025, %152, %1057 : i32 loc(#loc71)
    %1062 = arith.select %1025, %153, %1058 : i32 loc(#loc71)
    %1063 = arith.select %1025, %154, %1059 : i32 loc(#loc71)
    %1064 = arith.select %1025, %155, %1060 : i32 loc(#loc71)
    %1065 = arith.select %1025, %156, %c0_i32 : i32 loc(#loc71)
    %1066 = arith.select %1025, %157, %c0_i32 : i32 loc(#loc71)
    %1067 = arith.select %1025, %158, %c0_i32 : i32 loc(#loc71)
    %1068 = arith.select %1025, %159, %c0_i32 : i32 loc(#loc71)
    acir.next %1041 {aspect = "value", resource = "top.source$storage$0$0"} : i32 loc(#loc71)
    acir.next %1042 {aspect = "value", resource = "top.source$storage$0$1"} : i32 loc(#loc71)
    acir.next %1043 {aspect = "value", resource = "top.source$storage$0$2"} : i32 loc(#loc71)
    acir.next %1044 {aspect = "value", resource = "top.source$storage$0$3"} : i32 loc(#loc71)
    acir.next %1053 {aspect = "value", resource = "top.source$storage$1$0"} : i32 loc(#loc71)
    acir.next %1054 {aspect = "value", resource = "top.source$storage$1$1"} : i32 loc(#loc71)
    acir.next %1055 {aspect = "value", resource = "top.source$storage$1$2"} : i32 loc(#loc71)
    acir.next %1056 {aspect = "value", resource = "top.source$storage$1$3"} : i32 loc(#loc71)
    acir.next %1061 {aspect = "value", resource = "top.source$storage$2$0"} : i32 loc(#loc71)
    acir.next %1062 {aspect = "value", resource = "top.source$storage$2$1"} : i32 loc(#loc71)
    acir.next %1063 {aspect = "value", resource = "top.source$storage$2$2"} : i32 loc(#loc71)
    acir.next %1064 {aspect = "value", resource = "top.source$storage$2$3"} : i32 loc(#loc71)
    acir.next %1065 {aspect = "value", resource = "top.source$storage$3$0"} : i32 loc(#loc71)
    acir.next %1066 {aspect = "value", resource = "top.source$storage$3$1"} : i32 loc(#loc71)
    acir.next %1067 {aspect = "value", resource = "top.source$storage$3$2"} : i32 loc(#loc71)
    acir.next %1068 {aspect = "value", resource = "top.source$storage$3$3"} : i32 loc(#loc71)
    %1069 = arith.subi %142, %1024 : i32 loc(#loc71)
    acir.next %1069 {aspect = "value", resource = "top.source$count"} : i32 loc(#loc71)
    %1070 = arith.cmpi ult, %146, %c8_i32 : i32
    acir.effect %1021(%1070) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %1071 = arith.cmpi ne, %297, %c0_i32 : i32 loc(#loc72)
    %1072 = arith.cmpi ne, %297, %c1_i32 : i32 loc(#loc34)
    %1073 = arith.cmpi ne, %297, %c2_i32 : i32 loc(#loc35)
    %1074 = arith.cmpi ne, %297, %c3_i32 : i32 loc(#loc36)
    %1075 = arith.andi %1073, %1074 : i1 loc(#loc36)
    %1076 = arith.andi %1072, %1075 : i1 loc(#loc36)
    %1077 = arith.andi %1071, %1076 : i1 loc(#loc36)
    %1078 = arith.xori %1077, %true : i1 loc(#loc36)
    %1079 = arith.andi %160, %462 : i1
    %1080 = arith.andi %141, %1079 : i1
    %1081 = arith.andi %1, %1080 : i1
    %1082 = arith.andi %312, %1081 : i1
    acir.effect %1082(%1078) {branch = 58 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc36)
    %1083 = arith.xori %481, %true : i1
    %1084 = arith.select %481, %1083, %false : i1
    %1085 = arith.andi %462, %1084 : i1
    %1086 = arith.andi %160, %1085 : i1
    %1087 = arith.xori %1086, %true : i1
    acir.effect %1021(%1087) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %1088 = arith.cmpi ult, %147, %c8_i32 : i32 loc(#loc73)
    acir.effect %1021(%1088) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc73)
    %1089 = arith.cmpi ne, %284, %c0_i32 : i32 loc(#loc72)
    %1090 = arith.cmpi ne, %284, %c1_i32 : i32 loc(#loc34)
    %1091 = arith.cmpi ne, %284, %c2_i32 : i32 loc(#loc35)
    %1092 = arith.cmpi ne, %284, %c3_i32 : i32 loc(#loc36)
    %1093 = arith.andi %1091, %1092 : i1 loc(#loc36)
    %1094 = arith.andi %1090, %1093 : i1 loc(#loc36)
    %1095 = arith.andi %1089, %1094 : i1 loc(#loc36)
    %1096 = arith.xori %1095, %true : i1 loc(#loc36)
    %1097 = arith.andi %204, %509 : i1
    %1098 = arith.andi %141, %1097 : i1
    %1099 = arith.andi %1, %1098 : i1
    %1100 = arith.andi %312, %1099 : i1
    acir.effect %1100(%1096) {branch = 89 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand", "top.rob:lookup", "top.rob:$dispatch20"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc36)
    %1101 = arith.xori %528, %true : i1
    %1102 = arith.select %528, %1101, %false : i1
    %1103 = arith.andi %509, %1102 : i1
    %1104 = arith.andi %204, %1103 : i1
    %1105 = arith.xori %1104, %true : i1
    acir.effect %1021(%1105) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:operand"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> ()
    %1106 = arith.cmpi ult, %145, %c8_i32 : i32 loc(#loc74)
    acir.effect %1021(%1106) {branch = 31 : i64, frames = ["top.dispatch:dispatch", "top.rf:rename"], method = "check", port = 0 : i64, resource = "", txn = 6 : i64} : (i1, i1) -> () loc(#loc74)
    %1107 = arith.cmpi ne, %314, %c3_i32 : i32 loc(#loc40)
    %1108 = arith.andi %986, %1107 : i1 loc(#loc40)
    %1109 = arith.andi %955, %1108 : i1 loc(#loc40)
    %1110 = arith.andi %927, %1109 : i1 loc(#loc40)
    %1111 = arith.xori %1110, %true : i1 loc(#loc40)
    acir.effect %354(%1111) {branch = 109 : i64, frames = ["top.commit:commit", "top.rob:retire", "top.rob:$dispatch44"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> () loc(#loc40)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc2 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable1")
#loc3 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable3")
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":46:16)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:31)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:47)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:29)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:51)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:59)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:32)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:66)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:75)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:19)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:12)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:12)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:15)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:19)
#loc24 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7:index")
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:23)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:20)
#loc27 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13:rd")
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":155:13)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:20)
#loc30 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable1")
#loc31 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable2")
#loc32 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable3")
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:23)
#loc34 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable1")
#loc35 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable2")
#loc36 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable3")
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:48)
#loc38 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable1")
#loc39 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable2")
#loc40 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable3")
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":160:13)
#loc42 = loc("acir.binding:top.audit_body.0.call.56.2:saved")
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:35)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:12)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:28)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:44)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:39)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:16)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:16)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:20)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:40)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:17)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:40)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:17)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:66)
#loc57 = loc("acir.binding:top.scheduler_body:a_mul")
#loc58 = loc("acir.binding:top.scheduler_body:b_mul")
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:25)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":159:12)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:13)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":160:35)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:16)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:33)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:50)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":67:13)
#loc67 = loc("acir.binding:top.commit_body.0.call.44.1.0.call.44.3.0.call.44.20:$enable0")
#loc68 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.27.13.0.call.28.22:$enable0")
#loc69 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable0")
#loc70 = loc("acir.binding:top.cdb_body.0.call.11.8.0.call.11.19.0.call.11.17:$enable2")
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:16)
#loc72 = loc("acir.binding:top.dispatch_body.0.call.20.12.0.call.20.7.0.call.20.68.0.call.20.30:$enable0")
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:16)

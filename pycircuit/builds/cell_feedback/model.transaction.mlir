module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "CellFeedback", type_order = [], types = {}}, acir.stage = "transaction"} {
  %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [], schedule = [0, 1], stage = "transaction"} {
    acir.state @top.a : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.b : i32 {initial = 2 : i32, kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = 0 : i64} loc(#loc1)
    acir.path %true {branch = 0 : i64, frames = ["top:update_a"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top:update_a"], method = "read", port = 0 : i64, resource = "top.b", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc2)
    %0 = arith.addi %value, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc2)
    %1 = acir.fire "top.update_a#0" %ready {successors = [], txn = 0 : i64}
    acir.effect %1(%0) {branch = 0 : i64, frames = ["top:update_a"], method = "write", port = 0 : i64, resource = "top.a", txn = 0 : i64} : (i1, i32) -> () loc(#loc3)
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 1 : i64} loc(#loc4)
    acir.path %true {branch = 1 : i64, frames = ["top:update_b"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc4)
    %value_0, %ready_1 = acir.read(%true) {branch = 1 : i64, frames = ["top:update_b"], method = "read", port = 0 : i64, resource = "top.a", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc5)
    %2 = arith.addi %value_0, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc5)
    %3 = acir.fire "top.update_b#1" %ready_1 {successors = [], txn = 1 : i64}
    acir.effect %3(%2) {branch = 1 : i64, frames = ["top:update_b"], method = "write", port = 0 : i64, resource = "top.b", txn = 1 : i64} : (i1, i32) -> () loc(#loc6)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":20:9)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:13)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":21:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:13)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:9)

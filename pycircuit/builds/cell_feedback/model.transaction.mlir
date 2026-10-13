module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "CellFeedback", type_order = [], types = {}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [], schedule = [0, 1], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.a : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.b : i32 {initial = 2 : i32, kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = 0 : i64} loc(#loc1)
    acir.path %true {branch = 0 : i64, frames = ["top:update_a"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc1)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc2)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top:update_a"], method = "read", port = 0 : i64, resource = "top.b", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %0 = arith.addi %value, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc3)
    %1 = arith.andi %ready, %ready : i1 loc(#loc4)
    %2 = acir.fire "top.update_a#0" %1 {successors = [], txn = 0 : i64}
    acir.effect %2(%0) {branch = 0 : i64, frames = ["top:update_a"], method = "write", port = 0 : i64, resource = "top.a", txn = 0 : i64} : (i1, i32) -> () loc(#loc4)
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %true {branch = 1 : i64, frames = ["top:update_b"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc6)
    %value_1, %ready_2 = acir.read(%true) {branch = 1 : i64, frames = ["top:update_b"], method = "read", port = 0 : i64, resource = "top.a", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc7)
    %3 = arith.addi %value_1, %c1_i32_0 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc7)
    %4 = arith.andi %ready_2, %ready_2 : i1 loc(#loc8)
    %5 = acir.fire "top.update_b#1" %4 {successors = [], txn = 1 : i64}
    acir.effect %5(%3) {branch = 1 : i64, frames = ["top:update_b"], method = "write", port = 0 : i64, resource = "top.b", txn = 1 : i64} : (i1, i32) -> () loc(#loc8)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":20:9)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:17)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:13)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":21:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:17)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:13)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:9)

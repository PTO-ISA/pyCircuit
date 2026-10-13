module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "EHRShared", type_order = [], types = {}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 0 : i64, target_branches = [1], to = 0 : i64}, {from = 2 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 0 : i64, target_branches = [1], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [0], to = 1 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [0], to = 2 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [1], to = 0 : i64}], schedule = [3, 1, 2, 0], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.first_seen : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.second_seen : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.storage.state : i32 {initial = 1 : i32, kind = "ehr"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "ehr.read", port = 0 : i64, resource = "top.storage.state", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc1)
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc2)
    acir.path %true {branch = 1 : i64, frames = ["top.storage:late"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc2)
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc3)
    %0 = acir.fire "top.storage.late#0" %true {successors = [], txn = 0 : i64}
    acir.effect %0(%c9_i32) {branch = 1 : i64, frames = ["top.storage:late"], method = "ehr.write", port = 2 : i64, resource = "top.storage.state", txn = 0 : i64} : (i1, i32) -> () loc(#loc4)
    acir.path %true {branch = 0 : i64, frames = ["top.storage:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc5)
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = ["top.storage:read"], method = "ehr.read", port = 1 : i64, resource = "top.storage.state", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc6)
    acir.path %true {branch = 2 : i64, kind = "branch", txn = 1 : i64} loc(#loc7)
    acir.path %true {branch = 2 : i64, frames = ["top:first"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc7)
    %1 = arith.andi %ready_1, %ready_1 : i1 loc(#loc8)
    %2 = acir.fire "top.first#1" %1 {successors = [0], txn = 1 : i64}
    acir.effect %2(%value_0) {branch = 2 : i64, frames = ["top:first"], method = "write", port = 0 : i64, resource = "top.first_seen", txn = 1 : i64} : (i1, i32) -> () loc(#loc8)
    acir.path %true {branch = 3 : i64, kind = "branch", txn = 2 : i64} loc(#loc9)
    acir.path %true {branch = 3 : i64, frames = ["top:second"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc9)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc10)
    %3 = arith.addi %value_0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc11)
    %4 = arith.andi %ready_1, %ready_1 : i1 loc(#loc12)
    %5 = acir.fire "top.second#2" %4 {successors = [0], txn = 2 : i64}
    acir.effect %5(%3) {branch = 3 : i64, frames = ["top:second"], method = "write", port = 0 : i64, resource = "top.second_seen", txn = 2 : i64} : (i1, i32) -> () loc(#loc12)
    acir.path %true {branch = 4 : i64, kind = "branch", txn = 3 : i64} loc(#loc13)
    acir.path %true {branch = 4 : i64, frames = ["top.storage:early"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc13)
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc14)
    %6 = acir.fire "top.storage.early#3" %true {successors = [0, 1, 2], txn = 3 : i64}
    acir.effect %6(%c7_i32) {branch = 4 : i64, frames = ["top.storage:early"], method = "ehr.write", port = 0 : i64, resource = "top.storage.state", txn = 3 : i64} : (i1, i32) -> () loc(#loc15)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":42:9)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":43:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":20:17)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":20:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":44:18)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":16:16)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":45:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":35:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":46:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":39:31)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":39:23)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":39:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":47:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":12:17)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":12:9)

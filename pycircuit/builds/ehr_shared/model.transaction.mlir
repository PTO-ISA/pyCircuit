module attributes {acir.instances = [{definition = "src_ehr_shared_32e4aeac.EHRShared", name = "top", parent, rules = ["first", "second"]}, {definition = "src_ehr_shared_32e4aeac.Storage", name = "top.storage", parent = "top", rules = ["early", "late", "read"]}], acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "EHRShared", type_order = [], types = {}}, acir.stage = "transaction"} {
  %c7_i32 = arith.constant 7 : i32
  %c1_i32 = arith.constant 1 : i32
  %c9_i32 = arith.constant 9 : i32
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 2 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 1 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 2 : i64}, {from = 1 : i64, relation = "SB", resource = "Module contract", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 2 : i64, relation = "SB", resource = "Module contract", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [1], to = 1 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [1], to = 2 : i64}], schedule = [3, 1, 2, 0], stage = "transaction"} {
    acir.state @top.first_seen : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.second_seen : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.storage.state : i32 {initial = 1 : i32, kind = "ehr"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = 0 : i64} loc(#loc1)
    acir.path %true {branch = 0 : i64, frames = ["top.storage:late"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc1)
    %0 = acir.fire "top.storage.late#0" %true {successors = [], txn = 0 : i64}
    acir.effect %0(%c9_i32) {branch = 0 : i64, frames = ["top.storage:late"], method = "ehr.write", port = 2 : i64, resource = "top.storage.state", txn = 0 : i64} : (i1, i32) -> () loc(#loc2)
    acir.path %true {branch = 1 : i64, kind = "branch", txn = -1 : i64}
    acir.path %true {branch = 1 : i64, frames = ["top.storage:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value, %ready = acir.read(%true) {branch = 1 : i64, frames = ["top.storage:read"], method = "ehr.read", port = 1 : i64, resource = "top.storage.state", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %1 = acir.export %value : i32 -> i32 loc(#loc4)
    %2 = acir.export %ready : i1 -> i1
    acir.path %true {branch = 2 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %true {branch = 2 : i64, frames = ["top:first"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %3 = acir.fire "top.first#1" %ready {successors = [0], txn = 1 : i64}
    acir.effect %3(%1) {branch = 2 : i64, frames = ["top:first"], method = "write", port = 0 : i64, resource = "top.first_seen", txn = 1 : i64} : (i1, i32) -> () loc(#loc6)
    acir.path %true {branch = 3 : i64, kind = "branch", txn = 2 : i64} loc(#loc7)
    acir.path %true {branch = 3 : i64, frames = ["top:second"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc7)
    %4 = arith.addi %1, %c1_i32 : i32 loc(#loc8)
    %5 = acir.fire "top.second#2" %ready {successors = [0], txn = 2 : i64}
    acir.effect %5(%4) {branch = 3 : i64, frames = ["top:second"], method = "write", port = 0 : i64, resource = "top.second_seen", txn = 2 : i64} : (i1, i32) -> () loc(#loc9)
    acir.path %true {branch = 4 : i64, kind = "branch", txn = 3 : i64} loc(#loc10)
    acir.path %true {branch = 4 : i64, frames = ["top.storage:early"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc10)
    %6 = acir.fire "top.storage.early#3" %true {successors = [0, 1, 2], txn = 3 : i64}
    acir.effect %6(%c7_i32) {branch = 4 : i64, frames = ["top.storage:early"], method = "ehr.write", port = 0 : i64, resource = "top.storage.state", txn = 3 : i64} : (i1, i32) -> () loc(#loc11)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":39:9)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":20:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":16:16)
#loc4 = loc("acir.binding:top_body:shared")
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":41:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":32:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":42:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":36:23)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":36:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":43:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":12:9)

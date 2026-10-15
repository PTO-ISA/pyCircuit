module attributes {acir.instances = [{definition = "src_ehr_shared_32e4aeac.EHRShared", name = "top", parent, rules = ["first", "second"]}, {definition = "src_ehr_shared_32e4aeac.Storage", name = "top.storage", parent = "top", rules = ["early", "late", "read"]}], acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "EHRShared", type_order = [], types = {}}, acir.stage = "rtl"} {
  %c9_i32 = arith.constant 9 : i32
  %c1_i32 = arith.constant 1 : i32
  %c7_i32 = arith.constant 7 : i32
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 2 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 1 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 2 : i64}, {from = 1 : i64, relation = "SB", resource = "Module contract", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 2 : i64, relation = "SB", resource = "Module contract", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [1], to = 1 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [1], to = 2 : i64}], logical_states = [{kind = "cell", name = "top.first_seen", parts = [{path = [], resource = "top.first_seen$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.second_seen", parts = [{path = [], resource = "top.second_seen$value"}], prototype = 0 : i32}, {kind = "ehr", name = "top.storage.state", parts = [{path = [], resource = "top.storage.state$value"}], prototype = 1 : i32}], schedule = [3, 1, 2, 0], stage = "rtl"} {
    acir.state @top.first_seen$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.second_seen$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.storage.state$value : i32 {initial = 1 : i32, kind = "cell"}
    %0 = acir.fire "top.storage.late#0" %true {successors = [], txn = 0 : i64}
    %1 = acir.fire "top.storage.early#3" %true {successors = [0, 1, 2], txn = 3 : i64}
    %2 = acir.fire "top.first#1" %true {successors = [0], txn = 1 : i64}
    %3 = acir.fire "top.second#2" %true {successors = [0], txn = 2 : i64}
    %4 = acir.current {aspect = "value", resource = "top.storage.state$value"} : i32 loc(#loc1)
    %5 = arith.select %1, %c7_i32, %4 : i32 loc(#loc1)
    %6 = acir.export %5 : i32 -> i32 loc(#loc1)
    %7 = acir.current {aspect = "value", resource = "top.first_seen$value"} : i32
    %8 = arith.select %2, %6, %7 : i32
    acir.next %8 {aspect = "value", resource = "top.first_seen$value"} : i32
    %9 = arith.addi %6, %c1_i32 : i32
    %10 = acir.current {aspect = "value", resource = "top.second_seen$value"} : i32 loc(#loc2)
    %11 = arith.select %3, %9, %10 : i32 loc(#loc2)
    acir.next %11 {aspect = "value", resource = "top.second_seen$value"} : i32 loc(#loc2)
    %12 = arith.select %0, %c9_i32, %5 : i32
    acir.next %12 {aspect = "value", resource = "top.storage.state$value"} : i32
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":16:16)
#loc2 = loc("acir.binding:top_body:shared")

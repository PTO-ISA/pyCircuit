module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "EHRShared", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 2 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 1 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 2 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}], schedule = [3, 1, 2, 0], stage = "rtl"} {
    acir.state @top.first_seen : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.second_seen : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.storage.state : i32 {initial = 1 : i32, kind = "ehr"}
    %true = arith.constant true
    %false = arith.constant false
    %true_0 = arith.constant true
    %0 = acir.fire "top.storage.late#0" %true_0 {successors = [], txn = 0 : i64}
    %1 = acir.current {aspect = "value", resource = "top.storage.state"} : i32 loc(#loc1)
    %2 = acir.fire "top.storage.early#3" %true_0 {successors = [0, 1, 2], txn = 3 : i64}
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc2)
    %3 = arith.select %2, %c7_i32, %1 : i32 loc(#loc2)
    %4 = arith.andi %true, %true : i1 loc(#loc2)
    %5 = acir.fire "top.first#1" %4 {successors = [0], txn = 1 : i64} loc(#loc2)
    %6 = arith.andi %true, %true : i1 loc(#loc3)
    %7 = acir.fire "top.second#2" %6 {successors = [0], txn = 2 : i64} loc(#loc3)
    %8 = acir.current {aspect = "value", resource = "top.first_seen"} : i32 loc(#loc3)
    %9 = arith.select %5, %3, %8 : i32 loc(#loc3)
    acir.next %9 {aspect = "value", resource = "top.first_seen"} : i32 loc(#loc3)
    %10 = acir.current {aspect = "value", resource = "top.second_seen"} : i32 loc(#loc3)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc4)
    %11 = arith.addi %3, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc4)
    %12 = arith.select %7, %11, %10 : i32 loc(#loc4)
    acir.next %12 {aspect = "value", resource = "top.second_seen"} : i32 loc(#loc4)
    %13 = arith.select %2, %c7_i32, %1 : i32 loc(#loc4)
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc5)
    %14 = arith.select %0, %c9_i32, %13 : i32 loc(#loc5)
    acir.next %14 {aspect = "value", resource = "top.storage.state"} : i32 loc(#loc5)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":16:16)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":12:17)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":36:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":36:31)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":20:17)

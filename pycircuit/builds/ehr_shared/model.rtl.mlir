module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "EHRShared", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 2 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 1 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 1 : i64}, {from = 3 : i64, relation = "EHR.port", resource = "top.storage.state", source_branch = 4 : i64, target_branches = [1], to = 2 : i64}, {from = 3 : i64, relation = "SB", resource = "Module contract", source_branch = 4 : i64, target_branches = [0], to = 0 : i64}], logical_states = [{kind = "cell", name = "top.first_seen", parts = [{path = [], resource = "top.first_seen$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.second_seen", parts = [{path = [], resource = "top.second_seen$value"}], prototype = 0 : i32}, {kind = "ehr", name = "top.storage.state", parts = [{path = [], resource = "top.storage.state$value"}], prototype = 1 : i32}], schedule = [3, 1, 2, 0], stage = "rtl"} {
    acir.state @top.first_seen$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.second_seen$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.storage.state$value : i32 {initial = 1 : i32, kind = "cell"}
    %true = arith.constant true
    %0 = acir.fire "top.storage.late#0" %true {successors = [], txn = 0 : i64}
    %1 = acir.fire "top.storage.early#3" %true {successors = [0, 1, 2], txn = 3 : i64}
    %true_0 = arith.constant true
    %false = arith.constant false
    %2 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    %3 = acir.assert %2
    %4 = arith.andi %true_0, %3 : i1
    %5 = acir.fire "top.first#1" %4 {successors = [0], txn = 1 : i64}
    %6 = acir.fire "top.second#2" %4 {successors = [0], txn = 2 : i64}
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %7 = acir.current {aspect = "value", resource = "top.storage.state$value"} : i32 loc(#loc1)
    %8 = arith.select %1, %c7_i32, %7 : i32 loc(#loc1)
    %9 = acir.current {aspect = "value", resource = "top.first_seen$value"} : i32
    %10 = arith.select %5, %8, %9 : i32
    acir.next %10 {aspect = "value", resource = "top.first_seen$value"} : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %11 = arith.addi %8, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
    %12 = acir.current {aspect = "value", resource = "top.second_seen$value"} : i32
    %13 = arith.select %6, %11, %12 : i32
    acir.next %13 {aspect = "value", resource = "top.second_seen$value"} : i32
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32
    %14 = arith.select %1, %c7_i32, %7 : i32
    %15 = arith.select %0, %c9_i32, %14 : i32
    acir.next %15 {aspect = "value", resource = "top.storage.state$value"} : i32
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":16:16)

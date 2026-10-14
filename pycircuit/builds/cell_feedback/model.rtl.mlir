module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "CellFeedback", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [], logical_states = [{kind = "cell", name = "top.a", parts = [{path = [], resource = "top.a$value"}], prototype = 1 : i32}, {kind = "cell", name = "top.b", parts = [{path = [], resource = "top.b$value"}], prototype = 2 : i32}], schedule = [0, 1], stage = "rtl"} {
    acir.state @top.a$value : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.b$value : i32 {initial = 2 : i32, kind = "cell"}
    %true = arith.constant true
    %0 = acir.fire "top.update_a#0" %true {successors = [], txn = 0 : i64}
    %1 = acir.fire "top.update_b#1" %true {successors = [], txn = 1 : i64} loc(#loc1)
    %2 = acir.current {aspect = "value", resource = "top.b$value"} : i32 loc(#loc2)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %3 = arith.addi %2, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32
    %4 = acir.current {aspect = "value", resource = "top.a$value"} : i32 loc(#loc1)
    %5 = arith.select %0, %3, %4 : i32 loc(#loc1)
    acir.next %5 {aspect = "value", resource = "top.a$value"} : i32 loc(#loc1)
    %6 = arith.addi %4, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc1)
    %7 = arith.select %1, %6, %2 : i32 loc(#loc1)
    acir.next %7 {aspect = "value", resource = "top.b$value"} : i32 loc(#loc1)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:13)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:13)

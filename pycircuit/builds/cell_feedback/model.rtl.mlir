module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "CellFeedback", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [], schedule = [0, 1], stage = "rtl"} {
    acir.state @top.a : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.b : i32 {initial = 2 : i32, kind = "cell"}
    %true = arith.constant true
    %false = arith.constant false
    %0 = acir.current {aspect = "value", resource = "top.b"} : i32 loc(#loc1)
    %1 = arith.andi %true, %true : i1 loc(#loc1)
    %2 = acir.fire "top.update_a#0" %1 {successors = [], txn = 0 : i64} loc(#loc1)
    %3 = acir.current {aspect = "value", resource = "top.a"} : i32 loc(#loc2)
    %4 = arith.andi %true, %true : i1 loc(#loc2)
    %5 = acir.fire "top.update_b#1" %4 {successors = [], txn = 1 : i64} loc(#loc2)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc3)
    %6 = arith.addi %0, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc3)
    %7 = arith.select %2, %6, %3 : i32 loc(#loc3)
    acir.next %7 {aspect = "value", resource = "top.a"} : i32 loc(#loc3)
    %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc4)
    %8 = arith.addi %3, %c1_i32_0 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc4)
    %9 = arith.select %5, %8, %0 : i32 loc(#loc4)
    acir.next %9 {aspect = "value", resource = "top.b"} : i32 loc(#loc4)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:13)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:13)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:17)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:17)

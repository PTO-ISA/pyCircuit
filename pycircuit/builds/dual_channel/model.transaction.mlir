module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "DualChannel", type_order = [], types = {}}, acir.stage = "transaction"} {
  %c2_i32 = arith.constant 2 : i32
  %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
  %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [], schedule = [0, 1], stage = "transaction"} {
    acir.state @top.left_out : i32 {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.right_out : i32 {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.source : i32 {capacity = 1 : i64, initial = [7 : i32], kind = "queue"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    acir.path %true {branch = 0 : i64, frames = ["top.observer:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.observer:peek"], method = "peek", port = 0 : i64, resource = "top.source", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc2)
    %0 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    %1 = acir.assert %0
    %2 = arith.andi %ready, %1 : i1
    %3 = arith.addi %value, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc1)
    %4 = arith.cmpi ne, %3, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc3)
    %5 = arith.cmpi eq, %3, %c0_i32 : i32 loc(#loc3)
    %6 = arith.ori %5, %2 : i1
    %7 = arith.andi %6, %2 : i1 loc(#loc4)
    %8 = arith.andi %4, %7 : i1 loc(#loc4)
    acir.path %4 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc4)
    acir.path %4 {branch = 1 : i64, frames = ["top:left"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc4)
    %9 = arith.ori %5, %6 : i1 loc(#loc4)
    %10 = arith.andi %9, %8 : i1
    %11 = arith.andi %4, %10 : i1
    %12 = arith.xori %11, %true : i1
    %13 = arith.ori %12, %6 : i1
    %14 = arith.andi %8, %13 : i1
    acir.path %11 {branch = 2 : i64, kind = "branch", txn = 0 : i64}
    %value_0, %ready_1 = acir.read(%11) {branch = 2 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.left_out", txn = 0 : i64} : (i1) -> (i1, i1)
    %15 = arith.andi %6, %ready_1 : i1
    %16 = arith.ori %12, %15 : i1
    %17 = arith.andi %14, %16 : i1
    %18 = arith.xori %value_0, %true : i1
    %19 = arith.andi %6, %18 : i1
    %20 = arith.ori %12, %19 : i1
    %21 = arith.andi %17, %20 : i1
    %22 = acir.fire "top.left#0" %21 {successors = [], txn = 0 : i64}
    %23 = arith.andi %22, %11 : i1
    acir.effect %23(%3) {branch = 2 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.left_out", txn = 0 : i64} : (i1, i32) -> ()
    acir.path %4 {branch = 3 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %4 {branch = 3 : i64, frames = ["top:right"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %24 = arith.addi %value, %c2_i32 : i32 loc(#loc6)
    acir.path %11 {branch = 4 : i64, kind = "branch", txn = 1 : i64}
    %value_2, %ready_3 = acir.read(%11) {branch = 4 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.right_out", txn = 1 : i64} : (i1) -> (i1, i1)
    %25 = arith.andi %6, %ready_3 : i1
    %26 = arith.ori %12, %25 : i1
    %27 = arith.andi %14, %26 : i1
    %28 = arith.xori %value_2, %true : i1
    %29 = arith.andi %6, %28 : i1
    %30 = arith.ori %12, %29 : i1
    %31 = arith.andi %27, %30 : i1
    %32 = acir.fire "top.right#1" %31 {successors = [], txn = 1 : i64}
    %33 = arith.andi %32, %11 : i1
    acir.effect %33(%24) {branch = 4 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.right_out", txn = 1 : i64} : (i1, i32) -> ()
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:18)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":15:16)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":36:12)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":37:24)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":38:25)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":32:16)

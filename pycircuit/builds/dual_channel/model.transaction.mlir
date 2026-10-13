module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "DualChannel", type_order = [], types = {}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [], schedule = [0, 1], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.left_out : i32 {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.right_out : i32 {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.source : i32 {capacity = 1 : i64, initial = [7 : i32], kind = "queue"}
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc1)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc2)
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc3)
    acir.path %true {branch = 0 : i64, frames = ["top.observer:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc3)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.observer:peek"], method = "peek", port = 0 : i64, resource = "top.source", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc4)
    %0 = arith.addi %value, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc3)
    %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc5)
    %2 = arith.xori %1, %true : i1
    %3 = arith.xori %1, %true : i1
    %4 = arith.ori %3, %ready : i1
    %5 = arith.andi %4, %ready : i1 loc(#loc6)
    %6 = arith.andi %1, %5 : i1 loc(#loc6)
    acir.path %1 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc6)
    acir.path %1 {branch = 1 : i64, frames = ["top:left"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc6)
    %7 = arith.xori %1, %true : i1 loc(#loc6)
    %8 = arith.ori %7, %4 : i1 loc(#loc6)
    %9 = arith.andi %4, %ready : i1 loc(#loc6)
    %10 = arith.andi %1, %9 : i1 loc(#loc6)
    %11 = arith.andi %8, %10 : i1 loc(#loc7)
    %12 = arith.andi %1, %11 : i1 loc(#loc7)
    %13 = arith.andi %1, %12 : i1 loc(#loc7)
    %14 = arith.xori %13, %true : i1 loc(#loc7)
    %15 = arith.ori %14, %4 : i1 loc(#loc7)
    %16 = arith.andi %6, %15 : i1 loc(#loc7)
    acir.path %13 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc7)
    %value_0, %ready_1 = acir.read(%13) {branch = 2 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.left_out", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc7)
    %17 = arith.xori %13, %true : i1 loc(#loc7)
    %18 = arith.andi %4, %ready_1 : i1 loc(#loc7)
    %19 = arith.ori %17, %18 : i1 loc(#loc7)
    %20 = arith.andi %16, %19 : i1 loc(#loc7)
    %21 = arith.xori %value_0, %true : i1 loc(#loc7)
    %22 = arith.xori %13, %true : i1 loc(#loc7)
    %23 = arith.andi %4, %21 : i1 loc(#loc7)
    %24 = arith.ori %22, %23 : i1 loc(#loc7)
    %25 = arith.andi %20, %24 : i1 loc(#loc7)
    %26 = acir.fire "top.left#0" %25 {successors = [], txn = 0 : i64} loc(#loc7)
    %27 = arith.andi %26, %13 : i1 loc(#loc7)
    acir.effect %27(%0) {branch = 2 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.left_out", txn = 0 : i64} : (i1, i32) -> () loc(#loc7)
    %28 = arith.andi %4, %ready : i1 loc(#loc8)
    %29 = arith.andi %1, %28 : i1 loc(#loc8)
    acir.path %1 {branch = 3 : i64, kind = "branch", txn = 1 : i64} loc(#loc8)
    acir.path %1 {branch = 3 : i64, frames = ["top:right"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc8)
    %30 = arith.xori %1, %true : i1 loc(#loc8)
    %31 = arith.ori %30, %4 : i1 loc(#loc8)
    %32 = arith.andi %4, %ready : i1 loc(#loc8)
    %33 = arith.andi %1, %32 : i1 loc(#loc8)
    %c1_i32_2 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc9)
    %34 = arith.addi %0, %c1_i32_2 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc10)
    %35 = arith.andi %31, %33 : i1 loc(#loc11)
    %36 = arith.andi %1, %35 : i1 loc(#loc11)
    %37 = arith.andi %1, %36 : i1 loc(#loc11)
    %38 = arith.xori %37, %true : i1 loc(#loc11)
    %39 = arith.ori %38, %4 : i1 loc(#loc11)
    %40 = arith.andi %29, %39 : i1 loc(#loc11)
    acir.path %37 {branch = 4 : i64, kind = "branch", txn = 1 : i64} loc(#loc11)
    %value_3, %ready_4 = acir.read(%37) {branch = 4 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.right_out", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc11)
    %41 = arith.xori %37, %true : i1 loc(#loc11)
    %42 = arith.andi %4, %ready_4 : i1 loc(#loc11)
    %43 = arith.ori %41, %42 : i1 loc(#loc11)
    %44 = arith.andi %40, %43 : i1 loc(#loc11)
    %45 = arith.xori %value_3, %true : i1 loc(#loc11)
    %46 = arith.xori %37, %true : i1 loc(#loc11)
    %47 = arith.andi %4, %45 : i1 loc(#loc11)
    %48 = arith.ori %46, %47 : i1 loc(#loc11)
    %49 = arith.andi %44, %48 : i1 loc(#loc11)
    %50 = acir.fire "top.right#1" %49 {successors = [], txn = 1 : i64} loc(#loc11)
    %51 = arith.andi %50, %37 : i1 loc(#loc11)
    acir.effect %51(%34) {branch = 4 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.right_out", txn = 1 : i64} : (i1, i32) -> () loc(#loc11)
    %52 = arith.xori %2, %true : i1
    %53 = arith.ori %52, %ready : i1
    %54 = arith.ori %2, %1 : i1
    %55 = arith.xori %1, %true : i1
    %56 = arith.ori %55, %4 : i1
    %57 = arith.andi %53, %56 : i1
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":40:22)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":39:42)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":39:18)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":15:16)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":40:12)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":41:24)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":31:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":42:25)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:24)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:16)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:9)

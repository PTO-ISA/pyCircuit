module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, format_version = 9 : i64, top = "SignalLink", type_order = [], types = {}}, acir.stage = "transaction"} {
  %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
  %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
  %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
  %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 0 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.result", source_branch = 2 : i64, target_branches = [4], to = 2 : i64}], schedule = [0, 1, 2], stage = "transaction"} {
    acir.state @top.producer.count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.producer.offset : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.reader.last : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.result : i32 {initial = 0 : i32, kind = "signal"}
    %0 = acir.export %true {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
    %1 = arith.xori %0, %true : i1
    acir.path %0 {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%0) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.producer.count", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc1)
    %2 = arith.cmpi ult, %value, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc1)
    %3 = arith.andi %0, %ready : i1
    %4 = arith.ori %1, %3 : i1
    %5 = arith.select %0, %2, %false : i1
    %6 = arith.xori %5, %true : i1 loc(#loc2)
    %7 = arith.ori %6, %4 : i1
    %8 = arith.andi %5, %7 : i1 loc(#loc2)
    acir.path %5 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc2)
    acir.path %5 {branch = 1 : i64, frames = ["top.producer:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc2)
    %9 = arith.ori %6, %7 : i1 loc(#loc2)
    %value_0, %ready_1 = acir.read(%5) {branch = 1 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.count", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %10 = arith.andi %9, %ready_1 : i1 loc(#loc3)
    %11 = arith.ori %6, %10 : i1 loc(#loc3)
    %12 = arith.andi %8, %11 : i1 loc(#loc3)
    %value_2, %ready_3 = acir.read(%5) {branch = 1 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.offset", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc4)
    %13 = arith.andi %9, %ready_3 : i1 loc(#loc4)
    %14 = arith.ori %6, %13 : i1 loc(#loc4)
    %15 = arith.andi %12, %14 : i1 loc(#loc4)
    %16 = arith.andi %ready_1, %ready_3 : i1 loc(#loc3)
    %17 = arith.addi %value_0, %value_2 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc3)
    %18 = arith.andi %15, %11 : i1 loc(#loc5)
    %19 = arith.addi %value_0, %c1_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc5)
    %20 = arith.andi %18, %11 : i1 loc(#loc6)
    %21 = arith.andi %9, %16 : i1
    %22 = arith.andi %5, %21 : i1
    %23 = arith.xori %22, %true : i1
    %24 = arith.ori %23, %7 : i1
    %25 = arith.andi %20, %24 : i1
    acir.path %22 {branch = 2 : i64, kind = "branch", txn = 0 : i64}
    %26 = acir.fire "top.producer.advance#0" %25 {successors = [2], txn = 0 : i64}
    %27 = arith.andi %26, %5 : i1
    acir.effect %27(%19) {branch = 1 : i64, frames = ["top.producer:advance"], method = "write", port = 0 : i64, resource = "top.producer.count", txn = 0 : i64} : (i1, i32) -> () loc(#loc6)
    %28 = arith.andi %26, %22 : i1
    acir.effect %28(%17) {branch = 2 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.result", txn = 0 : i64} : (i1, i32) -> ()
    acir.path %true {branch = 3 : i64, kind = "branch", txn = 1 : i64} loc(#loc7)
    acir.path %true {branch = 3 : i64, frames = ["top:configure"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc7)
    acir.path %true {branch = 3 : i64, frames = ["top:configure", "top.producer:configure"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc8)
    %29 = acir.fire "top.configure#1" %true {successors = [], txn = 1 : i64}
    acir.effect %29(%c4_i32) {branch = 3 : i64, frames = ["top:configure", "top.producer:configure"], method = "write", port = 0 : i64, resource = "top.producer.offset", txn = 1 : i64} : (i1, i32) -> () loc(#loc9)
    acir.path %true {branch = 4 : i64, kind = "branch", txn = -1 : i64} loc(#loc10)
    %value_4, %ready_5 = acir.read(%true) {branch = 4 : i64, frames = [], method = "get", port = 0 : i64, resource = "top.result", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc10)
    %30 = arith.addi %value_4, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc11)
    %31 = arith.cmpi ne, %30, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc12)
    %32 = arith.cmpi eq, %30, %c0_i32 : i32 loc(#loc12)
    %33 = arith.ori %32, %ready_5 : i1
    %34 = arith.andi %33, %ready_5 : i1 loc(#loc13)
    %35 = arith.andi %31, %34 : i1 loc(#loc13)
    acir.path %31 {branch = 5 : i64, kind = "branch", txn = 2 : i64} loc(#loc13)
    acir.path %31 {branch = 5 : i64, frames = ["top.reader:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc13)
    %36 = arith.ori %32, %33 : i1 loc(#loc13)
    %37 = arith.andi %36, %35 : i1 loc(#loc14)
    %38 = arith.ori %32, %37 : i1 loc(#loc14)
    %39 = arith.andi %35, %38 : i1 loc(#loc14)
    %40 = acir.fire "top.reader.observe#2" %39 {successors = [], txn = 2 : i64}
    %41 = arith.andi %40, %31 : i1
    acir.effect %41(%30) {branch = 5 : i64, frames = ["top.reader:observe"], method = "write", port = 0 : i64, resource = "top.reader.last", txn = 2 : i64} : (i1, i32) -> () loc(#loc14)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:20)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:17)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:25)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:17)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":60:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":13:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":40:17)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:20)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:17)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":44:13)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":37:9)

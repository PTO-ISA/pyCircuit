module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, format_version = 9 : i64, top = "MixedPorts", type_order = [], types = {}}, acir.stage = "transaction"} {
  %c5_i32 = arith.constant 5 : i32
  %c2_i32 = arith.constant 2 : i32
  %c4_i32 = arith.constant 4 : i32
  %c3_i32 = arith.constant 3 : i32
  %c0_i32 = arith.constant 0 : i32
  %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 0 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.result", source_branch = 3 : i64, target_branches = [4], to = 1 : i64}], schedule = [0, 1], stage = "transaction"} {
    acir.state @top.accepted : i32 {capacity = 2 : i64, initial = [], kind = "queue"}
    acir.state @top.last_accepted : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.ledger.observed : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.observer.last : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.result : i32 {initial = 0 : i32, kind = "signal"}
    acir.state @top.source : i32 {capacity = 1 : i64, initial = [7 : i32], kind = "queue"}
    %0 = acir.export %true {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1
    acir.path %true {branch = 0 : i64, kind = "branch", txn = 0 : i64} loc(#loc1)
    acir.path %true {branch = 0 : i64, frames = ["top.router:route"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.router:route"], method = "peek", port = 0 : i64, resource = "top.source", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc2)
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = ["top.router:route"], method = "empty", port = 0 : i64, resource = "top.source", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc2)
    %1 = arith.andi %ready, %ready_1 : i1 loc(#loc2)
    %2 = arith.xori %value_0, %true : i1 loc(#loc2)
    %3 = arith.andi %1, %2 : i1 loc(#loc2)
    acir.path %true {branch = 0 : i64, frames = ["top.router:route", "top.ledger:record"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc3)
    %4 = arith.andi %3, %ready : i1 loc(#loc4)
    %5 = arith.xori %0, %true : i1
    %6 = arith.addi %value, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc5)
    %7 = acir.aggregate(%6, %value, %value, %true, %true, %true) {acir.id = "v15", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>"} : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc6)
    %8 = arith.andi %0, %ready : i1
    %9 = arith.ori %5, %8 : i1
    %10 = acir.aggregate() {acir.id = "v24", acir.source_type = "u32"} : () -> i32 loc(#loc7)
    %11 = acir.aggregate() {acir.id = "v26", acir.source_type = "u32"} : () -> i32 loc(#loc7)
    %12 = acir.aggregate(%10, %11, %value, %false, %false, %true) {acir.id = "v29", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>"} : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc7)
    %13 = arith.select %0, %5, %false : i1
    %14 = arith.andi %0, %9 : i1
    %15 = arith.andi %5, %ready : i1
    %16 = arith.ori %14, %15 : i1
    %17 = arith.select %0, %7, %12 : tuple<i32, i32, i32, i1, i1, i1>
    %18 = acir.unary %13 "not" {acir.source_type = "bool"} : i1 -> i1
    %19 = acir.extract %17[%c0_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %20 = acir.extract %17[%c3_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %21 = arith.xori %20, %true : i1
    %22 = arith.ori %21, %16 : i1
    %23 = arith.andi %4, %22 : i1
    acir.path %20 {branch = 1 : i64, kind = "branch", txn = 0 : i64}
    %value_2, %ready_3 = acir.read(%20) {branch = 1 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.accepted", txn = 0 : i64} : (i1) -> (i1, i1)
    %24 = arith.ori %21, %ready_3 : i1
    %25 = arith.andi %23, %24 : i1
    %26 = arith.xori %value_2, %true : i1
    %27 = arith.ori %21, %26 : i1
    %28 = arith.andi %25, %27 : i1
    %29 = acir.extract %17[%c1_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %30 = acir.extract %17[%c4_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %31 = arith.xori %30, %true : i1
    %32 = arith.ori %31, %16 : i1
    %33 = arith.andi %28, %32 : i1
    acir.path %30 {branch = 2 : i64, kind = "branch", txn = 0 : i64}
    %34 = acir.extract %17[%c2_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %35 = acir.extract %17[%c5_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %36 = arith.xori %35, %true : i1
    %37 = arith.ori %36, %16 : i1
    %38 = arith.andi %33, %37 : i1
    acir.path %35 {branch = 3 : i64, kind = "branch", txn = 0 : i64}
    %39 = acir.fire "top.router.route#0" %38 {successors = [1], txn = 0 : i64}
    acir.effect %39() {branch = 0 : i64, frames = ["top.router:route"], method = "pop", port = 0 : i64, resource = "top.source", txn = 0 : i64} : (i1) -> () loc(#loc2)
    acir.effect %39(%value) {branch = 0 : i64, frames = ["top.router:route", "top.ledger:record"], method = "write", port = 0 : i64, resource = "top.ledger.observed", txn = 0 : i64} : (i1, i32) -> () loc(#loc4)
    acir.effect %39(%18) {branch = 0 : i64, frames = ["top.router:route"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
    %40 = arith.andi %39, %20 : i1
    acir.effect %40(%19) {branch = 1 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.accepted", txn = 0 : i64} : (i1, i32) -> ()
    %41 = arith.andi %39, %30 : i1
    acir.effect %41(%29) {branch = 2 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.last_accepted", txn = 0 : i64} : (i1, i32) -> ()
    %42 = arith.andi %39, %35 : i1
    acir.effect %42(%34) {branch = 3 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.result", txn = 0 : i64} : (i1, i32) -> ()
    acir.path %true {branch = 4 : i64, kind = "branch", txn = 1 : i64} loc(#loc8)
    acir.path %true {branch = 4 : i64, frames = ["top.observer:sample"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc8)
    %value_4, %ready_5 = acir.read(%true) {branch = 4 : i64, frames = ["top.observer:sample"], method = "get", port = 0 : i64, resource = "top.result", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc9)
    %43 = acir.fire "top.observer.sample#1" %ready_5 {successors = [], txn = 1 : i64}
    acir.effect %43(%value_4) {branch = 4 : i64, frames = ["top.observer:sample"], method = "write", port = 0 : i64, resource = "top.observer.last", txn = 1 : i64} : (i1, i32) -> () loc(#loc10)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":42:16)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":35:17)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":36:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":12:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":38:21)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":38:13)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":39:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":25:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:16)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:9)

module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "SignalLink", type_order = [], types = {}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.result", source_branch = 3 : i64, target_branches = [4], to = 2 : i64}], schedule = [0, 1, 2], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.producer.count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.producer.offset : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.reader.last : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.result : i32 {initial = 0 : i32, kind = "signal"}
    %true_0 = arith.constant true loc(#loc1)
    %0 = acir.export %true_0 {acir.id = "v2", acir.source_type = "bool"} : i1 -> i1
    acir.path %true {branch = 0 : i64, kind = "branch", txn = 0 : i64} loc(#loc2)
    acir.path %true {branch = 0 : i64, frames = ["top:configure"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc2)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc3)
    acir.path %true {branch = 0 : i64, frames = ["top:configure", "top.producer:configure"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc4)
    %1 = acir.fire "top.configure#0" %true {successors = [], txn = 0 : i64}
    acir.effect %1(%c4_i32) {branch = 0 : i64, frames = ["top:configure", "top.producer:configure"], method = "write", port = 0 : i64, resource = "top.producer.offset", txn = 0 : i64} : (i1, i32) -> () loc(#loc5)
    %false_1 = arith.constant {acir.source_type = "bool"} false
    %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
    %2 = arith.xori %0, %true : i1
    %3 = arith.xori %0, %true : i1
    acir.path %0 {branch = 1 : i64, kind = "branch", txn = -1 : i64} loc(#loc6)
    %value, %ready = acir.read(%0) {branch = 1 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.producer.count", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc6)
    %4 = arith.cmpi ult, %value, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc6)
    %5 = arith.xori %2, %true : i1
    %6 = arith.ori %2, %0 : i1
    %7 = arith.xori %0, %true : i1
    %8 = arith.andi %0, %ready : i1
    %9 = arith.ori %2, %8 : i1
    %10 = arith.select %2, %false_1, %4 : i1
    %11 = arith.xori %10, %true : i1
    %12 = arith.xori %11, %true : i1
    %13 = arith.ori %12, %9 : i1
    %14 = arith.xori %10, %true : i1
    %15 = arith.ori %14, %9 : i1
    %16 = arith.andi %10, %15 : i1 loc(#loc7)
    acir.path %10 {branch = 2 : i64, kind = "branch", txn = 1 : i64} loc(#loc7)
    acir.path %10 {branch = 2 : i64, frames = ["top.producer:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc7)
    %17 = arith.xori %10, %true : i1 loc(#loc7)
    %18 = arith.ori %17, %15 : i1 loc(#loc7)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc8)
    %value_2, %ready_3 = acir.read(%10) {branch = 2 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.count", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc9)
    %19 = arith.xori %10, %true : i1 loc(#loc9)
    %20 = arith.andi %18, %ready_3 : i1 loc(#loc9)
    %21 = arith.ori %19, %20 : i1 loc(#loc9)
    %22 = arith.andi %16, %21 : i1 loc(#loc9)
    %value_4, %ready_5 = acir.read(%10) {branch = 2 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.offset", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc10)
    %23 = arith.xori %10, %true : i1 loc(#loc10)
    %24 = arith.andi %18, %ready_5 : i1 loc(#loc10)
    %25 = arith.ori %23, %24 : i1 loc(#loc10)
    %26 = arith.andi %22, %25 : i1 loc(#loc10)
    %27 = arith.andi %ready_3, %ready_5 : i1 loc(#loc9)
    %28 = arith.addi %value_2, %value_4 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc9)
    %value_6, %ready_7 = acir.read(%10) {branch = 2 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.count", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc11)
    %29 = arith.xori %10, %true : i1 loc(#loc11)
    %30 = arith.andi %18, %ready_7 : i1 loc(#loc11)
    %31 = arith.ori %29, %30 : i1 loc(#loc11)
    %32 = arith.andi %26, %31 : i1 loc(#loc11)
    %33 = arith.addi %value_6, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc11)
    %34 = arith.xori %10, %true : i1 loc(#loc12)
    %35 = arith.andi %18, %ready_7 : i1 loc(#loc12)
    %36 = arith.ori %34, %35 : i1 loc(#loc12)
    %37 = arith.andi %32, %36 : i1 loc(#loc12)
    %38 = arith.andi %18, %27 : i1 loc(#loc13)
    %39 = arith.andi %10, %38 : i1 loc(#loc13)
    %40 = arith.andi %10, %39 : i1 loc(#loc13)
    %41 = arith.xori %40, %true : i1 loc(#loc13)
    %42 = arith.ori %41, %15 : i1 loc(#loc13)
    %43 = arith.andi %37, %42 : i1 loc(#loc13)
    acir.path %40 {branch = 3 : i64, kind = "branch", txn = 1 : i64} loc(#loc13)
    %44 = acir.fire "top.producer.advance#1" %43 {successors = [2], txn = 1 : i64} loc(#loc13)
    %45 = arith.andi %44, %10 : i1 loc(#loc13)
    acir.effect %45(%33) {branch = 2 : i64, frames = ["top.producer:advance"], method = "write", port = 0 : i64, resource = "top.producer.count", txn = 1 : i64} : (i1, i32) -> () loc(#loc12)
    %46 = arith.andi %44, %40 : i1 loc(#loc13)
    acir.effect %46(%28) {branch = 3 : i64, frames = [], method = "set", port = 0 : i64, resource = "top.result", txn = 1 : i64} : (i1, i32) -> () loc(#loc13)
    %47 = arith.andi %11, %13 : i1 loc(#loc14)
    %48 = arith.andi %10, %15 : i1 loc(#loc14)
    %49 = arith.ori %47, %48 : i1 loc(#loc14)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc15)
    %c1_i32_8 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc16)
    acir.path %true {branch = 4 : i64, kind = "branch", txn = -1 : i64} loc(#loc17)
    %value_9, %ready_10 = acir.read(%true) {branch = 4 : i64, frames = [], method = "get", port = 0 : i64, resource = "top.result", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc17)
    %50 = arith.addi %value_9, %c1_i32_8 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc18)
    %51 = arith.cmpi ne, %50, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc19)
    %52 = arith.xori %51, %true : i1
    %53 = arith.xori %51, %true : i1
    %54 = arith.ori %53, %ready_10 : i1
    %55 = arith.andi %54, %ready_10 : i1 loc(#loc20)
    %56 = arith.andi %51, %55 : i1 loc(#loc20)
    acir.path %51 {branch = 5 : i64, kind = "branch", txn = 2 : i64} loc(#loc20)
    acir.path %51 {branch = 5 : i64, frames = ["top.reader:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc20)
    %57 = arith.xori %51, %true : i1 loc(#loc20)
    %58 = arith.ori %57, %54 : i1 loc(#loc20)
    %59 = arith.andi %54, %ready_10 : i1 loc(#loc20)
    %60 = arith.andi %51, %59 : i1 loc(#loc20)
    %61 = arith.xori %51, %true : i1 loc(#loc21)
    %62 = arith.andi %58, %60 : i1 loc(#loc21)
    %63 = arith.ori %61, %62 : i1 loc(#loc21)
    %64 = arith.andi %56, %63 : i1 loc(#loc21)
    %65 = arith.andi %51, %58 : i1
    %66 = acir.fire "top.reader.observe#2" %64 {successors = [], txn = 2 : i64}
    %67 = arith.andi %66, %51 : i1
    acir.effect %67(%50) {branch = 5 : i64, frames = ["top.reader:observe"], method = "write", port = 0 : i64, resource = "top.reader.last", txn = 2 : i64} : (i1, i32) -> () loc(#loc21)
    %68 = arith.xori %52, %true : i1
    %69 = arith.ori %68, %ready_10 : i1
    %70 = arith.ori %52, %51 : i1
    %71 = arith.xori %51, %true : i1
    %72 = arith.ori %71, %54 : i1
    %73 = arith.andi %69, %72 : i1
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":49:18)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":60:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:28)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":13:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:20)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:25)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:17)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:25)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:17)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:13)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:29)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:28)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":40:17)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:20)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:17)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":44:13)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":37:9)

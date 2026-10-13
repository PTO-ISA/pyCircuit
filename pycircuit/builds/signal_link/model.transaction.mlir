module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "SignalLink", type_order = [], types = {}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.producer.result", source_branch = 3 : i64, target_branches = [0], to = 2 : i64}], schedule = [0, 1, 2], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.producer.count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.producer.offset : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.producer.result : i32 {initial = 0 : i32, kind = "signal"}
    acir.state @top.reader.last : i32 {initial = 0 : i32, kind = "cell"}
    %true_0 = arith.constant true loc(#loc1)
    %0 = acir.export %true_0 {acir.id = "v2", acir.source_type = "bool"} : i1 -> i1
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc2)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "get", port = 0 : i64, resource = "top.producer.result", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc2)
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc3)
    acir.path %true {branch = 1 : i64, frames = ["top:configure"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc3)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc4)
    acir.path %true {branch = 1 : i64, frames = ["top:configure", "top.producer:configure"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    %1 = acir.fire "top.configure#0" %true {successors = [], txn = 0 : i64}
    acir.effect %1(%c4_i32) {branch = 1 : i64, frames = ["top:configure", "top.producer:configure"], method = "write", port = 0 : i64, resource = "top.producer.offset", txn = 0 : i64} : (i1, i32) -> () loc(#loc6)
    %value_1, %ready_2 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.reader.last", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc7)
    %false_3 = arith.constant {acir.source_type = "bool"} false
    %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
    %2 = arith.xori %0, %true : i1
    %3 = arith.xori %2, %true : i1
    %4 = arith.xori %0, %true : i1
    acir.path %0 {branch = 2 : i64, kind = "branch", txn = -1 : i64} loc(#loc8)
    %value_4, %ready_5 = acir.read(%0) {branch = 2 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.producer.count", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc8)
    %5 = arith.cmpi ult, %value_4, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc8)
    %6 = arith.xori %2, %true : i1
    %7 = arith.ori %2, %0 : i1
    %8 = arith.xori %0, %true : i1
    %9 = arith.andi %0, %ready_5 : i1
    %10 = arith.ori %2, %9 : i1
    %11 = arith.select %2, %false_3, %5 : i1
    %12 = arith.xori %11, %true : i1
    %13 = arith.xori %11, %true : i1
    %14 = arith.ori %13, %10 : i1
    %15 = arith.andi %11, %14 : i1 loc(#loc9)
    acir.path %11 {branch = 3 : i64, kind = "branch", txn = 1 : i64} loc(#loc9)
    acir.path %11 {branch = 3 : i64, frames = ["top.producer:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc9)
    %16 = arith.xori %11, %true : i1 loc(#loc9)
    %17 = arith.ori %16, %14 : i1 loc(#loc9)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc10)
    %value_6, %ready_7 = acir.read(%11) {branch = 3 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.count", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc11)
    %18 = arith.xori %11, %true : i1 loc(#loc11)
    %19 = arith.andi %17, %ready_7 : i1 loc(#loc11)
    %20 = arith.ori %18, %19 : i1 loc(#loc11)
    %21 = arith.andi %15, %20 : i1 loc(#loc11)
    %value_8, %ready_9 = acir.read(%11) {branch = 3 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.offset", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc12)
    %22 = arith.xori %11, %true : i1 loc(#loc12)
    %23 = arith.andi %17, %ready_9 : i1 loc(#loc12)
    %24 = arith.ori %22, %23 : i1 loc(#loc12)
    %25 = arith.andi %21, %24 : i1 loc(#loc12)
    %26 = arith.andi %ready_7, %ready_9 : i1 loc(#loc11)
    %27 = arith.addi %value_6, %value_8 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc11)
    %28 = arith.xori %11, %true : i1 loc(#loc13)
    %29 = arith.andi %17, %26 : i1 loc(#loc13)
    %30 = arith.ori %28, %29 : i1 loc(#loc13)
    %31 = arith.andi %25, %30 : i1 loc(#loc13)
    %value_10, %ready_11 = acir.read(%11) {branch = 3 : i64, frames = ["top.producer:advance"], method = "read", port = 0 : i64, resource = "top.producer.count", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc14)
    %32 = arith.xori %11, %true : i1 loc(#loc14)
    %33 = arith.andi %17, %ready_11 : i1 loc(#loc14)
    %34 = arith.ori %32, %33 : i1 loc(#loc14)
    %35 = arith.andi %31, %34 : i1 loc(#loc14)
    %36 = arith.addi %value_10, %c1_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc14)
    %37 = arith.xori %11, %true : i1 loc(#loc15)
    %38 = arith.andi %17, %ready_11 : i1 loc(#loc15)
    %39 = arith.ori %37, %38 : i1 loc(#loc15)
    %40 = arith.andi %35, %39 : i1 loc(#loc15)
    %41 = arith.andi %11, %17 : i1
    %42 = acir.fire "top.producer.advance#1" %40 {successors = [2], txn = 1 : i64}
    %43 = arith.andi %42, %11 : i1
    acir.effect %43(%27) {branch = 3 : i64, frames = ["top.producer:advance"], method = "set", port = 0 : i64, resource = "top.producer.result", txn = 1 : i64} : (i1, i32) -> () loc(#loc13)
    %44 = arith.andi %42, %11 : i1
    acir.effect %44(%36) {branch = 3 : i64, frames = ["top.producer:advance"], method = "write", port = 0 : i64, resource = "top.producer.count", txn = 1 : i64} : (i1, i32) -> () loc(#loc15)
    %45 = arith.xori %12, %true : i1
    %46 = arith.ori %45, %10 : i1
    %47 = arith.ori %12, %11 : i1
    %48 = arith.xori %11, %true : i1
    %49 = arith.ori %48, %14 : i1
    %50 = arith.andi %46, %49 : i1
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc16)
    %c1_i32_12 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc17)
    %value_13, %ready_14 = acir.read(%true) {branch = 0 : i64, frames = [], method = "get", port = 0 : i64, resource = "top.producer.result", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc18)
    %51 = arith.addi %value_13, %c1_i32_12 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc19)
    %52 = arith.cmpi ne, %51, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc20)
    %53 = arith.xori %52, %true : i1
    %54 = arith.xori %52, %true : i1
    %55 = arith.ori %54, %ready_14 : i1
    %56 = arith.andi %55, %ready_14 : i1 loc(#loc21)
    %57 = arith.andi %52, %56 : i1 loc(#loc21)
    acir.path %52 {branch = 4 : i64, kind = "branch", txn = 2 : i64} loc(#loc21)
    acir.path %52 {branch = 4 : i64, frames = ["top.reader:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc21)
    %58 = arith.xori %52, %true : i1 loc(#loc21)
    %59 = arith.ori %58, %55 : i1 loc(#loc21)
    %60 = arith.andi %55, %ready_14 : i1 loc(#loc21)
    %61 = arith.andi %52, %60 : i1 loc(#loc21)
    %62 = arith.xori %52, %true : i1 loc(#loc22)
    %63 = arith.andi %59, %61 : i1 loc(#loc22)
    %64 = arith.ori %62, %63 : i1 loc(#loc22)
    %65 = arith.andi %57, %64 : i1 loc(#loc22)
    %66 = arith.andi %52, %59 : i1
    %67 = acir.fire "top.reader.observe#2" %65 {successors = [], txn = 2 : i64}
    %68 = arith.andi %67, %52 : i1
    acir.effect %68(%51) {branch = 4 : i64, frames = ["top.reader:observe"], method = "write", port = 0 : i64, resource = "top.reader.last", txn = 2 : i64} : (i1, i32) -> () loc(#loc22)
    %69 = arith.xori %53, %true : i1
    %70 = arith.ori %69, %ready_14 : i1
    %71 = arith.ori %53, %52 : i1
    %72 = arith.xori %52, %true : i1
    %73 = arith.ori %72, %55 : i1
    %74 = arith.andi %70, %73 : i1
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":51:18)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":60:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":61:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":57:28)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":57:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":14:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":62:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:13)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:25)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:18)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:26)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:17)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:9)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":43:29)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:28)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:17)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:20)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":43:17)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":45:13)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":38:9)

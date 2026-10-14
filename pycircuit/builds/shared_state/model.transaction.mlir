module attributes {acir.config = {choose_left = true}, acir.model = {config = [{name = "choose_left", type = "bool"}], constants = {}, format_version = 9 : i64, top = "SharedState", type_order = [], types = {}}, acir.stage = "transaction"} {
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [], schedule = [0, 1, 2], stage = "transaction"} {
    acir.state @top.first : i32 {capacity = 1 : i64, initial = [3 : i32], kind = "queue"}
    acir.state @top.independent_out : i32 {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.left_out : i32 {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.right_out : i32 {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.second : i32 {capacity = 1 : i64, initial = [5 : i32], kind = "queue"}
    acir.state @top.separate.total : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.shared.total : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.third : i32 {capacity = 1 : i64, initial = [9 : i32], kind = "queue"}
    %0 = acir.export %true {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1
    acir.path %0 {branch = 0 : i64, kind = "branch", txn = 0 : i64} loc(#loc1)
    acir.path %0 {branch = 0 : i64, frames = ["top.left:transfer"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc1)
    acir.path %0 {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc2)
    %value, %ready = acir.read(%0) {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "read", port = 0 : i64, resource = "top.shared.total", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %1 = arith.xori %0, %true : i1 loc(#loc3)
    %2 = arith.ori %1, %ready : i1 loc(#loc3)
    %3 = arith.andi %0, %2 : i1 loc(#loc3)
    %value_0, %ready_1 = acir.read(%0) {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "peek", port = 0 : i64, resource = "top.first", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc4)
    %4 = arith.ori %1, %ready_1 : i1 loc(#loc4)
    %5 = arith.andi %3, %4 : i1 loc(#loc4)
    %6 = arith.andi %5, %4 : i1 loc(#loc4)
    %value_2, %ready_3 = acir.read(%0) {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "empty", port = 0 : i64, resource = "top.first", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc4)
    %7 = arith.ori %1, %ready_3 : i1 loc(#loc4)
    %8 = arith.andi %6, %7 : i1 loc(#loc4)
    %9 = arith.xori %value_2, %true : i1 loc(#loc4)
    %10 = arith.ori %1, %9 : i1 loc(#loc4)
    %11 = arith.andi %8, %10 : i1 loc(#loc4)
    %12 = arith.andi %ready, %ready_1 : i1 loc(#loc3)
    %13 = arith.addi %value, %value_0 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc3)
    %14 = arith.ori %1, %12 : i1 loc(#loc5)
    %15 = arith.andi %11, %14 : i1 loc(#loc5)
    %16 = arith.andi %15, %4 : i1 loc(#loc6)
    %17 = arith.andi %16, %4 : i1 loc(#loc6)
    acir.path %0 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc6)
    %value_4, %ready_5 = acir.read(%0) {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "empty", port = 0 : i64, resource = "top.first", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc6)
    %18 = arith.ori %1, %ready_5 : i1 loc(#loc6)
    %19 = arith.andi %17, %18 : i1 loc(#loc6)
    %20 = arith.xori %value_4, %true : i1 loc(#loc6)
    %21 = arith.ori %1, %20 : i1 loc(#loc6)
    %22 = arith.andi %19, %21 : i1 loc(#loc6)
    %23 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    %24 = arith.andi %0, %ready_1 : i1
    acir.path %24 {branch = 2 : i64, kind = "branch", txn = 0 : i64}
    %value_6, %ready_7 = acir.read(%24) {branch = 2 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.left_out", txn = 0 : i64} : (i1) -> (i1, i1)
    %25 = arith.xori %24, %true : i1
    %26 = arith.ori %25, %ready_7 : i1
    %27 = arith.andi %22, %26 : i1
    %28 = arith.xori %value_6, %true : i1
    %29 = arith.ori %25, %28 : i1
    %30 = arith.andi %27, %29 : i1
    %31 = acir.fire "top.left.transfer#0" %30 {successors = [], txn = 0 : i64}
    %32 = arith.andi %31, %0 : i1
    acir.effect %32() {branch = 1 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "pop", port = 0 : i64, resource = "top.first", txn = 0 : i64} : (i1) -> () loc(#loc4)
    acir.effect %32(%13) {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "write", port = 0 : i64, resource = "top.shared.total", txn = 0 : i64} : (i1, i32) -> () loc(#loc5)
    acir.effect %32(%23) {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
    %33 = arith.andi %31, %24 : i1
    acir.effect %33(%value_0) {branch = 2 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.left_out", txn = 0 : i64} : (i1, i32) -> ()
    %34 = acir.unary %true "not" {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1 loc(#loc7)
    %35 = acir.export %34 {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    acir.path %35 {branch = 3 : i64, kind = "branch", txn = 1 : i64} loc(#loc1)
    acir.path %35 {branch = 3 : i64, frames = ["top.right:transfer"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc1)
    acir.path %35 {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc2)
    %value_8, %ready_9 = acir.read(%35) {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "read", port = 0 : i64, resource = "top.shared.total", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %36 = arith.xori %35, %true : i1 loc(#loc3)
    %37 = arith.ori %36, %ready_9 : i1 loc(#loc3)
    %38 = arith.andi %35, %37 : i1 loc(#loc3)
    %value_10, %ready_11 = acir.read(%35) {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "peek", port = 0 : i64, resource = "top.second", txn = 1 : i64} : (i1) -> (i32, i1) loc(#loc4)
    %39 = arith.ori %36, %ready_11 : i1 loc(#loc4)
    %40 = arith.andi %38, %39 : i1 loc(#loc4)
    %41 = arith.andi %40, %39 : i1 loc(#loc4)
    %value_12, %ready_13 = acir.read(%35) {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "empty", port = 0 : i64, resource = "top.second", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc4)
    %42 = arith.ori %36, %ready_13 : i1 loc(#loc4)
    %43 = arith.andi %41, %42 : i1 loc(#loc4)
    %44 = arith.xori %value_12, %true : i1 loc(#loc4)
    %45 = arith.ori %36, %44 : i1 loc(#loc4)
    %46 = arith.andi %43, %45 : i1 loc(#loc4)
    %47 = arith.andi %ready_9, %ready_11 : i1 loc(#loc3)
    %48 = arith.addi %value_8, %value_10 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc3)
    %49 = arith.ori %36, %47 : i1 loc(#loc5)
    %50 = arith.andi %46, %49 : i1 loc(#loc5)
    %51 = arith.andi %50, %39 : i1 loc(#loc6)
    %52 = arith.andi %51, %39 : i1 loc(#loc6)
    acir.path %35 {branch = 4 : i64, kind = "branch", txn = 1 : i64} loc(#loc6)
    %value_14, %ready_15 = acir.read(%35) {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "empty", port = 0 : i64, resource = "top.second", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc6)
    %53 = arith.ori %36, %ready_15 : i1 loc(#loc6)
    %54 = arith.andi %52, %53 : i1 loc(#loc6)
    %55 = arith.xori %value_14, %true : i1 loc(#loc6)
    %56 = arith.ori %36, %55 : i1 loc(#loc6)
    %57 = arith.andi %54, %56 : i1 loc(#loc6)
    %58 = arith.andi %35, %ready_11 : i1
    acir.path %58 {branch = 5 : i64, kind = "branch", txn = 1 : i64}
    %value_16, %ready_17 = acir.read(%58) {branch = 5 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.right_out", txn = 1 : i64} : (i1) -> (i1, i1)
    %59 = arith.xori %58, %true : i1
    %60 = arith.ori %59, %ready_17 : i1
    %61 = arith.andi %57, %60 : i1
    %62 = arith.xori %value_16, %true : i1
    %63 = arith.ori %59, %62 : i1
    %64 = arith.andi %61, %63 : i1
    %65 = acir.fire "top.right.transfer#1" %64 {successors = [], txn = 1 : i64}
    %66 = arith.andi %65, %35 : i1
    acir.effect %66() {branch = 4 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "pop", port = 0 : i64, resource = "top.second", txn = 1 : i64} : (i1) -> () loc(#loc4)
    acir.effect %66(%48) {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "write", port = 0 : i64, resource = "top.shared.total", txn = 1 : i64} : (i1, i32) -> () loc(#loc5)
    acir.effect %66(%23) {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    %67 = arith.andi %65, %58 : i1
    acir.effect %67(%value_10) {branch = 5 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.right_out", txn = 1 : i64} : (i1, i32) -> ()
    %68 = acir.export %true {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
    acir.path %68 {branch = 6 : i64, kind = "branch", txn = 2 : i64} loc(#loc1)
    acir.path %68 {branch = 6 : i64, frames = ["top.independent:transfer"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc1)
    acir.path %68 {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc2)
    %value_18, %ready_19 = acir.read(%68) {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "read", port = 0 : i64, resource = "top.separate.total", txn = 2 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %69 = arith.xori %68, %true : i1 loc(#loc3)
    %70 = arith.ori %69, %ready_19 : i1 loc(#loc3)
    %71 = arith.andi %68, %70 : i1 loc(#loc3)
    %value_20, %ready_21 = acir.read(%68) {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "peek", port = 0 : i64, resource = "top.third", txn = 2 : i64} : (i1) -> (i32, i1) loc(#loc4)
    %72 = arith.ori %69, %ready_21 : i1 loc(#loc4)
    %73 = arith.andi %71, %72 : i1 loc(#loc4)
    %74 = arith.andi %73, %72 : i1 loc(#loc4)
    %value_22, %ready_23 = acir.read(%68) {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "empty", port = 0 : i64, resource = "top.third", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc4)
    %75 = arith.ori %69, %ready_23 : i1 loc(#loc4)
    %76 = arith.andi %74, %75 : i1 loc(#loc4)
    %77 = arith.xori %value_22, %true : i1 loc(#loc4)
    %78 = arith.ori %69, %77 : i1 loc(#loc4)
    %79 = arith.andi %76, %78 : i1 loc(#loc4)
    %80 = arith.andi %ready_19, %ready_21 : i1 loc(#loc3)
    %81 = arith.addi %value_18, %value_20 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc3)
    %82 = arith.ori %69, %80 : i1 loc(#loc5)
    %83 = arith.andi %79, %82 : i1 loc(#loc5)
    %84 = arith.andi %83, %72 : i1 loc(#loc6)
    %85 = arith.andi %84, %72 : i1 loc(#loc6)
    acir.path %68 {branch = 7 : i64, kind = "branch", txn = 2 : i64} loc(#loc6)
    %value_24, %ready_25 = acir.read(%68) {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "empty", port = 0 : i64, resource = "top.third", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc6)
    %86 = arith.ori %69, %ready_25 : i1 loc(#loc6)
    %87 = arith.andi %85, %86 : i1 loc(#loc6)
    %88 = arith.xori %value_24, %true : i1 loc(#loc6)
    %89 = arith.ori %69, %88 : i1 loc(#loc6)
    %90 = arith.andi %87, %89 : i1 loc(#loc6)
    %91 = arith.andi %68, %ready_21 : i1
    acir.path %91 {branch = 8 : i64, kind = "branch", txn = 2 : i64}
    %value_26, %ready_27 = acir.read(%91) {branch = 8 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.independent_out", txn = 2 : i64} : (i1) -> (i1, i1)
    %92 = arith.xori %91, %true : i1
    %93 = arith.ori %92, %ready_27 : i1
    %94 = arith.andi %90, %93 : i1
    %95 = arith.xori %value_26, %true : i1
    %96 = arith.ori %92, %95 : i1
    %97 = arith.andi %94, %96 : i1
    %98 = acir.fire "top.independent.transfer#2" %97 {successors = [], txn = 2 : i64}
    %99 = arith.andi %98, %68 : i1
    acir.effect %99() {branch = 7 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "pop", port = 0 : i64, resource = "top.third", txn = 2 : i64} : (i1) -> () loc(#loc4)
    acir.effect %99(%81) {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "write", port = 0 : i64, resource = "top.separate.total", txn = 2 : i64} : (i1, i32) -> () loc(#loc5)
    acir.effect %99(%23) {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "check", port = 0 : i64, resource = "", txn = 2 : i64} : (i1, i1) -> ()
    %100 = arith.andi %98, %91 : i1
    acir.effect %100(%value_20) {branch = 8 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.independent_out", txn = 2 : i64} : (i1, i32) -> ()
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":27:20)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":23:16)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:17)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:25)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":13:16)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":47:43)

module attributes {acir.instances = [{definition = "src_array_arbiter_1242b928.ArrayArbiter", name = "top", parent, rules = ["allocate", "clock", "drain", "issue"]}, {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.0", parent = "top", rules = ["allocate", "peek", "take", "wake"]}, {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.1", parent = "top", rules = ["allocate", "peek", "take", "wake"]}, {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.2", parent = "top", rules = ["allocate", "peek", "take", "wake"]}, {definition = "src_array_arbiter_1242b928.AgeMatrix", name = "top.age", parent = "top", rules = ["allocate", "choose", "release"]}], acir.model = {config = [], constants = {src_array_arbiter_1242b928.N = 3 : i64}, format_version = 9 : i64, top = "ArrayArbiter", type_order = ["src_array_arbiter_1242b928.Entry"], types = {src_array_arbiter_1242b928.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}, acir.stage = "transaction"} {
  %c2_i64 = arith.constant 2 : i64
  %c1_i64 = arith.constant 1 : i64
  %c4_i32 = arith.constant 4 : i32
  %c5_i32 = arith.constant 5 : i32
  %c3_i32 = arith.constant 3 : i32
  %c20_i32 = arith.constant 20 : i32
  %c10_i32 = arith.constant 10 : i32
  %c0_i64 = arith.constant 0 : i64
  %c3_i64 = arith.constant 3 : i64
  %c30_i32 = arith.constant 30 : i32
  %c2_i32 = arith.constant 2 : i32
  %c1_i32 = arith.constant 1 : i32
  %c0_i32 = arith.constant 0 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.output", source_branch = 26 : i64, target_branches = [16], to = 3 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.output", source_branch = 26 : i64, target_branches = [16], to = 3 : i64}], schedule = [0, 1, 2, 4, 5, 3, 6], stage = "transaction"} {
    acir.state @top.age.older : !acir.array<i1, [3, 3]> {initial = [[false, false, false], [false, false, false], [false, false, false]], kind = "cell"}
    acir.state @top.age.used : !acir.array<i1, [3]> {initial = [false, false, false], kind = "cell"}
    acir.state @top.cycle : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.issued : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.last : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.output : i32 {capacity = 1 : i64, initial = [99 : i32], kind = "queue"}
    acir.state @top.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.slots.0.state : !acir.struct<"src_array_arbiter_1242b928.Entry"> {initial = [false, false, 0 : i32], kind = "cell"}
    acir.state @top.slots.1.state : !acir.struct<"src_array_arbiter_1242b928.Entry"> {initial = [false, false, 0 : i32], kind = "cell"}
    acir.state @top.slots.2.state : !acir.struct<"src_array_arbiter_1242b928.Entry"> {initial = [false, false, 0 : i32], kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc1)
    %0 = arith.cmpi eq, %value, %c0_i32 : i32 loc(#loc1)
    %1 = arith.cmpi ne, %value, %c0_i32 : i32 loc(#loc1)
    acir.path %1 {branch = 1 : i64, kind = "branch", txn = -1 : i64} loc(#loc2)
    %value_0, %ready_1 = acir.read(%1) {branch = 1 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc2)
    %2 = arith.cmpi eq, %value_0, %c1_i32 : i32 loc(#loc2)
    %3 = arith.cmpi ne, %value_0, %c1_i32 : i32 loc(#loc2)
    %4 = arith.andi %1, %3 : i1
    acir.path %4 {branch = 2 : i64, kind = "branch", txn = -1 : i64} loc(#loc3)
    %value_2, %ready_3 = acir.read(%4) {branch = 2 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %5 = arith.cmpi eq, %value_2, %c2_i32 : i32 loc(#loc3)
    %6 = arith.andi %4, %5 : i1
    acir.path %6 {branch = 3 : i64, kind = "branch", txn = 0 : i64} loc(#loc4)
    acir.path %6 {branch = 3 : i64, frames = ["top:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc4)
    acir.path %6 {branch = 3 : i64, frames = ["top:allocate", "top:$dispatch0"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    acir.path %false {branch = 4 : i64, kind = "branch", txn = 0 : i64} loc(#loc5)
    acir.path %false {branch = 4 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    %7 = acir.aggregate(%true, %true, %c30_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc6)
    acir.path %6 {branch = 5 : i64, kind = "branch", txn = 0 : i64} loc(#loc5)
    acir.path %6 {branch = 5 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    %value_4, %ready_5 = acir.read(%6) {branch = 5 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 0 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc7)
    %8 = acir.extract %value_4 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc7)
    acir.path %false {branch = 6 : i64, kind = "branch", txn = 0 : i64} loc(#loc5)
    acir.path %false {branch = 6 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    acir.path %6 {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc8)
    %value_6, %ready_7 = acir.read(%6) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 0 : i64} : (i1) -> (!acir.array<i1, [3]>, i1)
    %9 = acir.bind %value_6 : (!acir.array<i1, [3]>) -> !acir.array<i1, [3]> loc(#loc9)
    %value_8, %ready_9 = acir.read(%6) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 0 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1)
    %10 = acir.bind %value_8 : (!acir.array<i1, [3, 3]>) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %11 = "acir.array.index"(%9, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %12 = acir.update %10, %11[%c0_i64, %c1_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %13 = acir.update %12, %false[%c1_i64, %c0_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %14 = "acir.array.index"(%9, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %15 = acir.update %13, %14[%c1_i64, %c1_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %16 = acir.update %15, %false[%c1_i64, %c1_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %17 = "acir.array.index"(%9, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %18 = acir.update %16, %17[%c2_i64, %c1_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %19 = acir.update %18, %false[%c1_i64, %c2_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %20 = acir.update %9, %true[%c1_i64] {path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc9)
    %21 = arith.xori %8, %true : i1 loc(#loc12)
    %22 = arith.andi %5, %21 : i1 loc(#loc12)
    %23 = arith.andi %3, %22 : i1 loc(#loc12)
    %24 = arith.andi %1, %23 : i1 loc(#loc12)
    %25 = acir.fire "top.allocate#0" %24 {successors = [], txn = 0 : i64}
    acir.effect %false(%7) {branch = 4 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 0 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    %26 = arith.andi %3, %5 : i1
    %27 = arith.andi %1, %26 : i1
    %28 = arith.andi %25, %27 : i1
    acir.effect %28(%7) {branch = 5 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 0 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    acir.effect %false(%7) {branch = 6 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 0 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    acir.effect %28(%20) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 0 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc13)
    acir.effect %28(%19) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.older", txn = 0 : i64} : (i1, !acir.array<i1, [3, 3]>) -> () loc(#loc14)
    %29 = arith.andi %1, %2 : i1
    acir.path %29 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc15)
    acir.path %29 {branch = 7 : i64, frames = ["top:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc15)
    acir.path %29 {branch = 7 : i64, frames = ["top:allocate", "top:$dispatch0"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    acir.path %false {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %false {branch = 8 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %30 = acir.aggregate(%true, %true, %c10_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc6)
    acir.path %false {branch = 9 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %false {branch = 9 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    acir.path %29 {branch = 10 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %29 {branch = 10 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %value_10, %ready_11 = acir.read(%29) {branch = 10 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc7)
    %31 = acir.extract %value_10 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc7)
    acir.path %29 {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc8)
    %value_12, %ready_13 = acir.read(%29) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 1 : i64} : (i1) -> (!acir.array<i1, [3]>, i1)
    %32 = acir.bind %value_12 : (!acir.array<i1, [3]>) -> !acir.array<i1, [3]> loc(#loc9)
    %value_14, %ready_15 = acir.read(%29) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 1 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1)
    %33 = acir.bind %value_14 : (!acir.array<i1, [3, 3]>) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %34 = "acir.array.index"(%32, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %35 = acir.update %33, %34[%c0_i64, %c0_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %36 = acir.update %35, %false[%c0_i64, %c0_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %37 = "acir.array.index"(%32, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %38 = acir.update %36, %37[%c1_i64, %c0_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %39 = acir.update %38, %false[%c0_i64, %c1_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %40 = "acir.array.index"(%32, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %41 = acir.update %39, %40[%c2_i64, %c0_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %42 = acir.update %41, %false[%c0_i64, %c2_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %43 = acir.update %32, %true[%c0_i64] {path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc9)
    %44 = arith.xori %31, %true : i1 loc(#loc12)
    %45 = arith.andi %2, %44 : i1 loc(#loc12)
    %46 = arith.andi %1, %45 : i1 loc(#loc12)
    %47 = acir.fire "top.allocate#1" %46 {successors = [], txn = 1 : i64}
    acir.effect %false(%30) {branch = 8 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 1 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    acir.effect %false(%30) {branch = 9 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 1 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    %48 = arith.andi %47, %29 : i1
    acir.effect %48(%30) {branch = 10 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 1 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    acir.effect %48(%43) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 1 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc13)
    acir.effect %48(%42) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.older", txn = 1 : i64} : (i1, !acir.array<i1, [3, 3]>) -> () loc(#loc14)
    acir.path %0 {branch = 11 : i64, kind = "branch", txn = 2 : i64} loc(#loc16)
    acir.path %0 {branch = 11 : i64, frames = ["top:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc16)
    acir.path %0 {branch = 11 : i64, frames = ["top:allocate", "top:$dispatch0"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    acir.path %0 {branch = 12 : i64, kind = "branch", txn = 2 : i64} loc(#loc5)
    acir.path %0 {branch = 12 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    %value_16, %ready_17 = acir.read(%0) {branch = 12 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 2 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc7)
    %49 = acir.extract %value_16 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc7)
    %50 = acir.aggregate(%true, %false, %c20_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc6)
    acir.path %false {branch = 13 : i64, kind = "branch", txn = 2 : i64} loc(#loc5)
    acir.path %false {branch = 13 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    acir.path %false {branch = 14 : i64, kind = "branch", txn = 2 : i64} loc(#loc5)
    acir.path %false {branch = 14 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    acir.path %0 {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc8)
    %value_18, %ready_19 = acir.read(%0) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 2 : i64} : (i1) -> (!acir.array<i1, [3]>, i1)
    %51 = acir.bind %value_18 : (!acir.array<i1, [3]>) -> !acir.array<i1, [3]> loc(#loc9)
    %value_20, %ready_21 = acir.read(%0) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 2 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1)
    %52 = acir.bind %value_20 : (!acir.array<i1, [3, 3]>) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %53 = "acir.array.index"(%51, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %54 = acir.update %52, %53[%c0_i64, %c2_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %55 = acir.update %54, %false[%c2_i64, %c0_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %56 = "acir.array.index"(%51, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %57 = acir.update %55, %56[%c1_i64, %c2_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %58 = acir.update %57, %false[%c2_i64, %c1_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %59 = "acir.array.index"(%51, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc11)
    %60 = acir.update %58, %59[%c2_i64, %c2_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %61 = acir.update %60, %false[%c2_i64, %c2_i64] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc10)
    %62 = acir.update %51, %true[%c2_i64] {path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc9)
    %63 = arith.xori %49, %true : i1 loc(#loc12)
    %64 = arith.andi %0, %63 : i1 loc(#loc12)
    %65 = acir.fire "top.allocate#2" %64 {successors = [], txn = 2 : i64}
    %66 = arith.andi %65, %0 : i1
    acir.effect %66(%50) {branch = 12 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 2 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    acir.effect %false(%50) {branch = 13 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 2 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    acir.effect %false(%50) {branch = 14 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 2 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc12)
    acir.effect %66(%62) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 2 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc13)
    acir.effect %66(%61) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.older", txn = 2 : i64} : (i1, !acir.array<i1, [3, 3]>) -> () loc(#loc14)
    acir.path %true {branch = 0 : i64, frames = ["top.slots.0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc17)
    %value_22, %ready_23 = acir.read(%true) {branch = 0 : i64, frames = ["top.slots.0:peek"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc18)
    %67 = acir.export %value_22 : !acir.struct<"src_array_arbiter_1242b928.Entry"> -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    %68 = acir.export %true : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.slots.1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc17)
    %value_24, %ready_25 = acir.read(%true) {branch = 0 : i64, frames = ["top.slots.1:peek"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc18)
    %69 = acir.export %value_24 : !acir.struct<"src_array_arbiter_1242b928.Entry"> -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    %70 = acir.export %true : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.slots.2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc17)
    %value_26, %ready_27 = acir.read(%true) {branch = 0 : i64, frames = ["top.slots.2:peek"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc18)
    %71 = acir.export %value_26 : !acir.struct<"src_array_arbiter_1242b928.Entry"> -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    %72 = acir.export %true : i1 -> i1
    %73 = arith.andi %68, %70 : i1
    %74 = arith.andi %73, %72 : i1
    %75 = acir.aggregate(%67, %69, %71) : (!acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> !acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]> loc(#loc19)
    %value_28, %ready_29 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc20)
    %76 = arith.cmpi uge, %value_28, %c3_i32 : i32 loc(#loc20)
    %77 = "acir.array.domain"(%75) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>) -> !acir.mask<[3]> loc(#loc21)
    %78 = "acir.array.index"(%75, %c0_i64) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i64) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc21)
    %79 = "acir.array.index"(%77, %c0_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc21)
    %80 = acir.extract %78 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc22)
    %81 = acir.extract %78 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc23)
    %82 = arith.andi %80, %81 : i1
    %83 = arith.xori %80, %true : i1
    %84 = arith.ori %83, %74 : i1
    %85 = arith.andi %74, %84 : i1
    %86 = arith.andi %79, %82 : i1
    %87 = arith.xori %79, %true : i1
    %88 = arith.ori %87, %85 : i1
    %89 = arith.andi %74, %88 : i1
    %90 = "acir.array.index"(%75, %c1_i64) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i64) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc21)
    %91 = "acir.array.index"(%77, %c1_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc21)
    %92 = acir.extract %90 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc22)
    %93 = acir.extract %90 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc23)
    %94 = arith.andi %92, %93 : i1
    %95 = arith.xori %92, %true : i1
    %96 = arith.ori %95, %74 : i1
    %97 = arith.andi %74, %96 : i1
    %98 = arith.andi %91, %94 : i1
    %99 = arith.xori %91, %true : i1
    %100 = arith.ori %99, %97 : i1
    %101 = arith.andi %74, %100 : i1
    %102 = "acir.array.index"(%75, %c2_i64) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i64) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc21)
    %103 = "acir.array.index"(%77, %c2_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc21)
    %104 = acir.extract %102 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc22)
    %105 = acir.extract %102 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc23)
    %106 = arith.andi %104, %105 : i1
    %107 = arith.xori %104, %true : i1
    %108 = arith.ori %107, %74 : i1
    %109 = arith.andi %74, %108 : i1
    %110 = arith.andi %103, %106 : i1
    %111 = arith.xori %103, %true : i1
    %112 = arith.ori %111, %109 : i1
    %113 = arith.andi %74, %112 : i1
    %114 = arith.andi %89, %101 : i1 loc(#loc21)
    %115 = arith.andi %114, %113 : i1 loc(#loc21)
    %116 = acir.aggregate(%86, %98, %110) : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc21)
    %117 = "acir.array.select"(%75, %116) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, !acir.mask<[3]>) -> !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>> loc(#loc24)
    %118 = arith.andi %76, %115 : i1 loc(#loc25)
    acir.path %76 {branch = 15 : i64, kind = "branch", txn = 3 : i64} loc(#loc25)
    acir.path %76 {branch = 15 : i64, frames = ["top:issue"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc25)
    acir.path %76 {branch = 15 : i64, frames = ["top:issue", "top.age:choose"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc26)
    %119 = "acir.array.domain"(%117) : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> !acir.mask<[3]> loc(#loc27)
    %120 = "acir.array.index"(%119, %c0_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc27)
    %121 = "acir.array.index"(%119, %c1_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc27)
    %122 = "acir.array.index"(%119, %c2_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc27)
    %123 = acir.aggregate(%120, %121, %122) : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc28)
    %value_30, %ready_31 = acir.read(%76) {branch = 15 : i64, frames = ["top:issue", "top.age:choose"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 3 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1)
    %124 = acir.bind %value_30 : (!acir.array<i1, [3, 3]>) -> !acir.array<i1, [3, 3]> loc(#loc29)
    %125 = "acir.array.index"(%123, %c0_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc30)
    %126 = arith.cmpi ult, %value_28, %c3_i32 : i32 loc(#loc20)
    %127 = arith.ori %126, %115 : i1
    %128 = arith.andi %118, %127 : i1
    %129 = "acir.array.index"(%124, %c0_i64) : (!acir.array<i1, [3, 3]>, i64) -> !acir.array<i1, [3]> loc(#loc31)
    %130 = "acir.array.index"(%129, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %131 = arith.andi %125, %130 : i1
    %132 = arith.xori %131, %true : i1 loc(#loc32)
    %133 = "acir.array.index"(%123, %c1_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc30)
    %134 = "acir.array.index"(%124, %c1_i64) : (!acir.array<i1, [3, 3]>, i64) -> !acir.array<i1, [3]> loc(#loc31)
    %135 = "acir.array.index"(%134, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %136 = arith.andi %133, %135 : i1
    %137 = arith.select %136, %false, %132 : i1 loc(#loc32)
    %138 = "acir.array.index"(%123, %c2_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc30)
    %139 = "acir.array.index"(%124, %c2_i64) : (!acir.array<i1, [3, 3]>, i64) -> !acir.array<i1, [3]> loc(#loc31)
    %140 = "acir.array.index"(%139, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %141 = arith.andi %138, %140 : i1
    %142 = arith.select %141, %false, %137 : i1 loc(#loc32)
    %143 = arith.select %125, %142, %false : i1 loc(#loc33)
    %144 = "acir.array.index"(%129, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %145 = arith.andi %125, %144 : i1
    %146 = arith.xori %145, %true : i1 loc(#loc32)
    %147 = "acir.array.index"(%134, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %148 = arith.andi %133, %147 : i1
    %149 = arith.select %148, %false, %146 : i1 loc(#loc32)
    %150 = "acir.array.index"(%139, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %151 = arith.andi %138, %150 : i1
    %152 = arith.select %151, %false, %149 : i1 loc(#loc32)
    %153 = arith.select %133, %152, %false : i1
    %154 = arith.select %153, %true, %143 : i1 loc(#loc33)
    %155 = arith.extui %153 : i1 to i32 loc(#loc34)
    %156 = "acir.array.index"(%129, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %157 = arith.andi %125, %156 : i1
    %158 = arith.xori %157, %true : i1 loc(#loc32)
    %159 = "acir.array.index"(%134, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %160 = arith.andi %133, %159 : i1
    %161 = arith.select %160, %false, %158 : i1 loc(#loc32)
    %162 = "acir.array.index"(%139, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc31)
    %163 = arith.andi %138, %162 : i1
    %164 = arith.select %163, %false, %161 : i1 loc(#loc32)
    %165 = arith.select %138, %164, %false : i1
    %166 = arith.select %165, %true, %154 : i1 loc(#loc33)
    %167 = arith.select %165, %c2_i32, %155 : i32 loc(#loc34)
    %168 = arith.andi %76, %166 : i1
    %169 = arith.xori %168, %true : i1 loc(#loc35)
    %170 = arith.ori %169, %115 : i1
    %171 = arith.cmpi eq, %167, %c0_i32 : i32 loc(#loc36)
    %172 = arith.cmpi eq, %167, %c1_i32 : i32 loc(#loc37)
    %173 = arith.cmpi eq, %167, %c2_i32 : i32 loc(#loc38)
    acir.path %168 {branch = 16 : i64, kind = "branch", txn = 3 : i64}
    acir.path %168 {branch = 16 : i64, frames = ["top:issue", "top:$dispatch10"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64}
    %174 = arith.select %171, %true, %172 : i1
    %175 = arith.select %174, %true, %173 : i1
    %176 = arith.andi %115, %175 : i1 loc(#loc35)
    %177 = arith.andi %170, %176 : i1 loc(#loc35)
    %178 = arith.ori %169, %177 : i1 loc(#loc35)
    %179 = arith.andi %128, %178 : i1 loc(#loc35)
    %180 = acir.aggregate() : () -> i32
    %181 = arith.andi %178, %115 : i1
    %182 = arith.ori %169, %181 : i1
    %183 = arith.andi %179, %182 : i1
    %184 = arith.cmpi ne, %167, %c0_i32 : i32 loc(#loc36)
    %185 = arith.andi %168, %184 : i1
    %186 = arith.xori %185, %true : i1
    %187 = arith.ori %186, %181 : i1
    %188 = arith.andi %187, %115 : i1
    %189 = arith.ori %186, %188 : i1
    %190 = arith.andi %183, %189 : i1
    %191 = arith.cmpi ne, %167, %c1_i32 : i32 loc(#loc37)
    %192 = arith.andi %185, %191 : i1
    %193 = arith.xori %192, %true : i1
    %194 = arith.ori %193, %188 : i1
    %195 = arith.cmpi ne, %167, %c2_i32 : i32 loc(#loc38)
    %196 = arith.andi %194, %115 : i1
    %197 = arith.ori %193, %196 : i1
    %198 = arith.andi %190, %197 : i1
    %199 = arith.andi %192, %173 : i1
    %200 = arith.xori %199, %true : i1 loc(#loc35)
    %201 = arith.ori %200, %196 : i1
    acir.path %199 {branch = 17 : i64, kind = "branch", txn = 3 : i64} loc(#loc35)
    acir.path %199 {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc35)
    %202 = arith.ori %200, %201 : i1 loc(#loc35)
    %value_32, %ready_33 = acir.read(%199) {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc39)
    %203 = arith.ori %200, %202 : i1 loc(#loc39)
    %204 = arith.andi %198, %203 : i1 loc(#loc39)
    %205 = acir.extract %value_32 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc39)
    %206 = arith.andi %199, %205 : i1
    %207 = arith.xori %206, %true : i1 loc(#loc40)
    %208 = arith.ori %207, %202 : i1
    acir.path %206 {branch = 18 : i64, kind = "branch", txn = 3 : i64} loc(#loc40)
    %value_34, %ready_35 = acir.read(%206) {branch = 18 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc40)
    %209 = arith.ori %207, %208 : i1 loc(#loc40)
    %210 = arith.andi %204, %209 : i1 loc(#loc40)
    %211 = acir.extract %value_34 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc40)
    %212 = arith.andi %205, %211 : i1
    %213 = arith.andi %201, %212 : i1 loc(#loc39)
    %214 = arith.ori %200, %213 : i1 loc(#loc39)
    %215 = arith.andi %210, %214 : i1 loc(#loc39)
    %value_36, %ready_37 = acir.read(%199) {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc41)
    %216 = arith.ori %200, %214 : i1 loc(#loc41)
    %217 = arith.andi %215, %216 : i1 loc(#loc41)
    %218 = acir.extract %value_36 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc42)
    %219 = acir.aggregate(%false, %false, %c0_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc43)
    %220 = arith.select %173, %218, %180 : i32
    %221 = arith.ori %195, %212 : i1
    %222 = arith.andi %115, %221 : i1
    %223 = arith.andi %185, %172 : i1
    %224 = arith.xori %223, %true : i1 loc(#loc35)
    %225 = arith.ori %224, %188 : i1
    acir.path %223 {branch = 19 : i64, kind = "branch", txn = 3 : i64} loc(#loc35)
    acir.path %223 {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc35)
    %226 = arith.ori %224, %225 : i1 loc(#loc35)
    %value_38, %ready_39 = acir.read(%223) {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc39)
    %227 = arith.ori %224, %226 : i1 loc(#loc39)
    %228 = arith.andi %217, %227 : i1 loc(#loc39)
    %229 = acir.extract %value_38 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc39)
    %230 = arith.andi %223, %229 : i1
    %231 = arith.xori %230, %true : i1 loc(#loc40)
    %232 = arith.ori %231, %226 : i1
    acir.path %230 {branch = 20 : i64, kind = "branch", txn = 3 : i64} loc(#loc40)
    %value_40, %ready_41 = acir.read(%230) {branch = 20 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc40)
    %233 = arith.ori %231, %232 : i1 loc(#loc40)
    %234 = arith.andi %228, %233 : i1 loc(#loc40)
    %235 = acir.extract %value_40 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc40)
    %236 = arith.andi %229, %235 : i1
    %237 = arith.andi %225, %236 : i1 loc(#loc39)
    %238 = arith.ori %224, %237 : i1 loc(#loc39)
    %239 = arith.andi %234, %238 : i1 loc(#loc39)
    %value_42, %ready_43 = acir.read(%223) {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc41)
    %240 = arith.ori %224, %238 : i1 loc(#loc41)
    %241 = arith.andi %239, %240 : i1 loc(#loc41)
    %242 = acir.extract %value_42 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc44)
    %243 = arith.ori %172, %115 : i1
    %244 = arith.andi %115, %243 : i1
    %245 = arith.andi %191, %195 : i1
    %246 = arith.select %172, %242, %220 : i32
    %247 = arith.select %172, %236, %222 : i1
    %248 = arith.andi %115, %247 : i1
    %249 = arith.andi %248, %244 : i1
    %250 = arith.andi %168, %171 : i1
    %251 = arith.xori %250, %true : i1 loc(#loc35)
    %252 = arith.ori %251, %181 : i1
    acir.path %250 {branch = 21 : i64, kind = "branch", txn = 3 : i64} loc(#loc35)
    acir.path %250 {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc35)
    %253 = arith.ori %251, %252 : i1 loc(#loc35)
    %value_44, %ready_45 = acir.read(%250) {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc39)
    %254 = arith.ori %251, %253 : i1 loc(#loc39)
    %255 = arith.andi %241, %254 : i1 loc(#loc39)
    %256 = acir.extract %value_44 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc39)
    %257 = arith.andi %250, %256 : i1
    %258 = arith.xori %257, %true : i1 loc(#loc40)
    %259 = arith.ori %258, %253 : i1
    acir.path %257 {branch = 22 : i64, kind = "branch", txn = 3 : i64} loc(#loc40)
    %value_46, %ready_47 = acir.read(%257) {branch = 22 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc40)
    %260 = arith.ori %258, %259 : i1 loc(#loc40)
    %261 = arith.andi %255, %260 : i1 loc(#loc40)
    %262 = acir.extract %value_46 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc40)
    %263 = arith.andi %256, %262 : i1
    %264 = arith.andi %252, %263 : i1 loc(#loc39)
    %265 = arith.ori %251, %264 : i1 loc(#loc39)
    %266 = arith.andi %261, %265 : i1 loc(#loc39)
    %value_48, %ready_49 = acir.read(%250) {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc41)
    %267 = arith.ori %251, %265 : i1 loc(#loc41)
    %268 = arith.andi %266, %267 : i1 loc(#loc41)
    %269 = acir.extract %value_48 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc45)
    %270 = arith.ori %171, %244 : i1
    %271 = arith.andi %115, %270 : i1
    %272 = arith.andi %184, %245 : i1
    %273 = arith.select %171, %269, %246 : i32 loc(#loc46)
    %274 = arith.select %171, %263, %249 : i1
    %275 = arith.andi %115, %274 : i1
    %276 = arith.xori %272, %true : i1
    %277 = arith.andi %178, %271 : i1
    %278 = arith.ori %169, %277 : i1
    %279 = arith.andi %268, %278 : i1
    %280 = arith.andi %275, %271 : i1
    %281 = arith.andi %280, %176 : i1
    acir.path %168 {branch = 16 : i64, frames = ["top:issue", "top.age:release"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc47)
    %282 = arith.ori %169, %170 : i1 loc(#loc47)
    %value_50, %ready_51 = acir.read(%168) {branch = 16 : i64, frames = ["top:issue", "top.age:release"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 3 : i64} : (i1) -> (!acir.array<i1, [3]>, i1)
    %283 = arith.ori %169, %282 : i1
    %284 = arith.andi %279, %283 : i1
    %285 = acir.bind %value_50 : (!acir.array<i1, [3]>) -> !acir.array<i1, [3]> loc(#loc48)
    %286 = arith.extui %167 : i32 to i64
    %287 = arith.cmpi ult, %286, %c3_i64 : i64
    %288 = arith.andi %115, %287 : i1
    %289 = arith.select %287, %286, %c0_i64 : i64
    %290 = acir.update %285, %false[%289] {path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc48)
    %291 = arith.andi %282, %288 : i1 loc(#loc49)
    %292 = arith.ori %169, %291 : i1 loc(#loc49)
    %293 = arith.andi %284, %292 : i1 loc(#loc49)
    %294 = arith.andi %170, %281 : i1 loc(#loc50)
    %295 = arith.ori %169, %294 : i1 loc(#loc50)
    %296 = arith.andi %293, %295 : i1 loc(#loc50)
    %value_52, %ready_53 = acir.read(%168) {branch = 16 : i64, frames = ["top:issue"], method = "full", port = 0 : i64, resource = "top.output", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc50)
    %297 = arith.andi %170, %ready_53 : i1 loc(#loc50)
    %298 = arith.ori %169, %297 : i1 loc(#loc50)
    %299 = arith.andi %296, %298 : i1 loc(#loc50)
    %300 = arith.xori %value_52, %true : i1 loc(#loc50)
    %301 = arith.andi %170, %300 : i1 loc(#loc50)
    %302 = arith.ori %169, %301 : i1 loc(#loc50)
    %303 = arith.andi %299, %302 : i1 loc(#loc50)
    %value_54, %ready_55 = acir.read(%168) {branch = 16 : i64, frames = ["top:issue"], method = "read", port = 0 : i64, resource = "top.issued", txn = 3 : i64} : (i1) -> (i32, i1) loc(#loc51)
    %304 = arith.andi %303, %282 : i1 loc(#loc51)
    %305 = arith.addi %value_54, %c1_i32 : i32 loc(#loc51)
    %306 = acir.fire "top.issue#3" %304 {successors = [], txn = 3 : i64}
    %307 = arith.xori %140, %true : i1
    %308 = arith.xori %135, %true : i1
    %309 = arith.xori %130, %true : i1
    %310 = arith.andi %191, %173 : i1
    %311 = arith.andi %184, %310 : i1
    %312 = arith.andi %309, %311 : i1
    %313 = arith.andi %308, %312 : i1
    %314 = arith.andi %307, %313 : i1
    %315 = arith.select %144, %314, %311 : i1
    %316 = arith.select %147, %314, %315 : i1
    %317 = arith.select %150, %314, %316 : i1
    %318 = arith.xori %150, %true : i1
    %319 = arith.xori %147, %true : i1
    %320 = arith.andi %319, %311 : i1
    %321 = arith.andi %318, %320 : i1
    %322 = arith.select %125, %317, %321 : i1
    %323 = arith.andi %307, %312 : i1
    %324 = arith.andi %125, %323 : i1
    %325 = arith.select %133, %322, %324 : i1
    %326 = arith.select %156, %317, %311 : i1
    %327 = arith.select %125, %326, %311 : i1
    %328 = arith.select %159, %322, %327 : i1
    %329 = arith.select %156, %323, %311 : i1
    %330 = arith.select %125, %329, %311 : i1
    %331 = arith.select %133, %328, %330 : i1
    %332 = arith.select %162, %325, %331 : i1
    %333 = arith.select %144, %313, %311 : i1
    %334 = arith.select %147, %313, %333 : i1
    %335 = arith.select %125, %334, %320 : i1
    %336 = arith.andi %125, %312 : i1
    %337 = arith.select %133, %335, %336 : i1
    %338 = arith.select %138, %332, %337 : i1
    %339 = arith.andi %76, %338 : i1
    %340 = arith.andi %306, %339 : i1
    acir.effect %340(%219) {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc52)
    %341 = arith.andi %184, %172 : i1
    %342 = arith.andi %309, %341 : i1
    %343 = arith.andi %308, %342 : i1
    %344 = arith.andi %307, %343 : i1
    %345 = arith.select %144, %344, %341 : i1
    %346 = arith.select %147, %344, %345 : i1
    %347 = arith.select %150, %344, %346 : i1
    %348 = arith.andi %319, %341 : i1
    %349 = arith.andi %318, %348 : i1
    %350 = arith.select %125, %347, %349 : i1
    %351 = arith.andi %307, %342 : i1
    %352 = arith.andi %125, %351 : i1
    %353 = arith.select %133, %350, %352 : i1
    %354 = arith.select %156, %347, %341 : i1
    %355 = arith.select %125, %354, %341 : i1
    %356 = arith.select %159, %350, %355 : i1
    %357 = arith.select %156, %351, %341 : i1
    %358 = arith.select %125, %357, %341 : i1
    %359 = arith.select %133, %356, %358 : i1
    %360 = arith.select %162, %353, %359 : i1
    %361 = arith.select %144, %343, %341 : i1
    %362 = arith.select %147, %343, %361 : i1
    %363 = arith.select %125, %362, %348 : i1
    %364 = arith.andi %125, %342 : i1
    %365 = arith.select %133, %363, %364 : i1
    %366 = arith.select %138, %360, %365 : i1
    %367 = arith.andi %76, %366 : i1
    %368 = arith.andi %306, %367 : i1
    acir.effect %368(%219) {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc52)
    %369 = arith.andi %309, %171 : i1
    %370 = arith.andi %308, %369 : i1
    %371 = arith.andi %307, %370 : i1
    %372 = arith.select %144, %371, %171 : i1
    %373 = arith.select %147, %371, %372 : i1
    %374 = arith.select %150, %371, %373 : i1
    %375 = arith.andi %319, %171 : i1
    %376 = arith.andi %318, %375 : i1
    %377 = arith.select %125, %374, %376 : i1
    %378 = arith.andi %307, %369 : i1
    %379 = arith.andi %125, %378 : i1
    %380 = arith.select %133, %377, %379 : i1
    %381 = arith.select %156, %374, %171 : i1
    %382 = arith.select %125, %381, %171 : i1
    %383 = arith.select %159, %377, %382 : i1
    %384 = arith.select %156, %378, %171 : i1
    %385 = arith.select %125, %384, %171 : i1
    %386 = arith.select %133, %383, %385 : i1
    %387 = arith.select %162, %380, %386 : i1
    %388 = arith.select %144, %370, %171 : i1
    %389 = arith.select %147, %370, %388 : i1
    %390 = arith.select %125, %389, %375 : i1
    %391 = arith.andi %125, %369 : i1
    %392 = arith.select %133, %390, %391 : i1
    %393 = arith.select %138, %387, %392 : i1
    %394 = arith.andi %76, %393 : i1
    %395 = arith.andi %306, %394 : i1
    acir.effect %395(%219) {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc52)
    %396 = arith.andi %308, %309 : i1
    %397 = arith.andi %307, %396 : i1
    %398 = arith.xori %144, %true : i1
    %399 = arith.ori %398, %397 : i1
    %400 = arith.select %147, %397, %399 : i1
    %401 = arith.select %150, %397, %400 : i1
    %402 = arith.andi %318, %319 : i1
    %403 = arith.select %125, %401, %402 : i1
    %404 = arith.andi %307, %309 : i1
    %405 = arith.andi %125, %404 : i1
    %406 = arith.select %133, %403, %405 : i1
    %407 = arith.xori %125, %true : i1
    %408 = arith.xori %156, %true : i1
    %409 = arith.ori %408, %401 : i1
    %410 = arith.ori %407, %409 : i1
    %411 = arith.select %159, %403, %410 : i1
    %412 = arith.ori %408, %404 : i1
    %413 = arith.ori %407, %412 : i1
    %414 = arith.select %133, %411, %413 : i1
    %415 = arith.select %162, %406, %414 : i1
    %416 = arith.ori %398, %396 : i1
    %417 = arith.select %147, %396, %416 : i1
    %418 = arith.select %125, %417, %319 : i1
    %419 = arith.andi %125, %309 : i1
    %420 = arith.select %133, %418, %419 : i1
    %421 = arith.select %138, %415, %420 : i1
    %422 = arith.andi %76, %421 : i1
    %423 = arith.andi %306, %422 : i1
    acir.effect %423(%276) {branch = 16 : i64, frames = ["top:issue", "top:$dispatch10"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
    acir.effect %423(%290) {branch = 16 : i64, frames = ["top:issue", "top.age:release"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 3 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc49)
    acir.effect %423(%273) {branch = 16 : i64, frames = ["top:issue"], method = "push", port = 0 : i64, resource = "top.output", txn = 3 : i64} : (i1, i32) -> () loc(#loc50)
    acir.effect %423(%305) {branch = 16 : i64, frames = ["top:issue"], method = "write", port = 0 : i64, resource = "top.issued", txn = 3 : i64} : (i1, i32) -> () loc(#loc53)
    %value_56, %ready_57 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc54)
    %424 = arith.cmpi eq, %value_56, %c5_i32 : i32 loc(#loc54)
    acir.path %424 {branch = 23 : i64, kind = "branch", txn = 4 : i64} loc(#loc55)
    acir.path %424 {branch = 23 : i64, frames = ["top.slots.2:wake"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc55)
    %value_58, %ready_59 = acir.read(%424) {branch = 23 : i64, frames = ["top.slots.2:wake"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 4 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1)
    %425 = acir.bind %value_58 : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc56)
    %426 = acir.extract %425 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc57)
    %427 = arith.andi %424, %426 : i1
    %428 = acir.extract %425 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc58)
    %429 = acir.aggregate(%426, %true, %428) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc56)
    acir.path %427 {branch = 24 : i64, kind = "branch", txn = 4 : i64} loc(#loc59)
    %430 = acir.fire "top.slots.2.wake#4" %424 {successors = [], txn = 4 : i64}
    %431 = arith.andi %430, %427 : i1
    acir.effect %431(%429) {branch = 24 : i64, frames = ["top.slots.2:wake"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 4 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc59)
    %value_60, %ready_61 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc60)
    %432 = arith.cmpi uge, %value_60, %c4_i32 : i32 loc(#loc60)
    acir.path %432 {branch = 25 : i64, kind = "branch", txn = -1 : i64} loc(#loc61)
    %value_62, %ready_63 = acir.read(%432) {branch = 25 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc61)
    %433 = arith.remui %value_62, %c2_i32 : i32 loc(#loc61)
    %434 = arith.cmpi eq, %433, %c0_i32 : i32 loc(#loc61)
    %435 = arith.andi %432, %434 : i1
    acir.path %435 {branch = 26 : i64, kind = "branch", txn = 5 : i64} loc(#loc62)
    acir.path %435 {branch = 26 : i64, frames = ["top:drain"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc62)
    %value_64, %ready_65 = acir.read(%435) {branch = 26 : i64, frames = ["top:drain"], method = "peek", port = 0 : i64, resource = "top.output", txn = 5 : i64} : (i1) -> (i32, i1) loc(#loc63)
    %value_66, %ready_67 = acir.read(%435) {branch = 26 : i64, frames = ["top:drain"], method = "read", port = 0 : i64, resource = "top.retired", txn = 5 : i64} : (i1) -> (i32, i1) loc(#loc64)
    %436 = arith.addi %value_66, %c1_i32 : i32 loc(#loc64)
    %437 = arith.andi %434, %ready_65 : i1 loc(#loc65)
    %438 = arith.andi %432, %437 : i1 loc(#loc65)
    %439 = acir.fire "top.drain#5" %438 {successors = [3], txn = 5 : i64}
    %440 = arith.andi %439, %435 : i1
    acir.effect %440() {branch = 26 : i64, frames = ["top:drain"], method = "pop", port = 0 : i64, resource = "top.output", txn = 5 : i64} : (i1) -> () loc(#loc63)
    acir.effect %440(%value_64) {branch = 26 : i64, frames = ["top:drain"], method = "write", port = 0 : i64, resource = "top.last", txn = 5 : i64} : (i1, i32) -> () loc(#loc65)
    acir.effect %440(%436) {branch = 26 : i64, frames = ["top:drain"], method = "write", port = 0 : i64, resource = "top.retired", txn = 5 : i64} : (i1, i32) -> () loc(#loc66)
    acir.path %true {branch = 27 : i64, kind = "branch", txn = 6 : i64} loc(#loc67)
    acir.path %true {branch = 27 : i64, frames = ["top:clock"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc67)
    %value_68, %ready_69 = acir.read(%true) {branch = 27 : i64, frames = ["top:clock"], method = "read", port = 0 : i64, resource = "top.cycle", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc68)
    %441 = arith.addi %value_68, %c1_i32 : i32 loc(#loc68)
    %442 = acir.fire "top.clock#6" %true {successors = [], txn = 6 : i64}
    acir.effect %442(%441) {branch = 27 : i64, frames = ["top:clock"], method = "write", port = 0 : i64, resource = "top.cycle", txn = 6 : i64} : (i1, i32) -> () loc(#loc69)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":115:12)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":117:14)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":119:14)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:13)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":93:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:17)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:24)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":94:9)
#loc9 = loc("acir.binding:top_body.0.call.0.8.0.call.4.13:flags")
#loc10 = loc("acir.binding:top_body.0.call.0.8.0.call.4.13:matrix")
#loc11 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:35)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":55:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":56:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:13)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:13)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":121:29)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":26:16)
#loc19 = loc("acir.binding:top_body:entries")
#loc20 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":122:12)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:27)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:55)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:71)
#loc24 = loc("acir.binding:top_body.0.call.9.94:candidates")
#loc25 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:13)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":98:27)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":66:16)
#loc28 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:mask")
#loc29 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:matrix")
#loc30 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:20)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:32)
#loc32 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:oldest")
#loc33 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:found")
#loc34 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:position")
#loc35 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":100:21)
#loc36 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable0")
#loc37 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable1")
#loc38 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable2")
#loc39 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:20)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:36)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":37:17)
#loc42 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17.0.call.12.22:value")
#loc43 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:17)
#loc44 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17.0.call.11.19:value")
#loc45 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17.0.call.10.16:value")
#loc46 = loc("acir.binding:top_body.0.call.9.94:value")
#loc47 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":101:13)
#loc48 = loc("acir.binding:top_body.0.call.9.94.0.call.14.19:flags")
#loc49 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":62:9)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":102:13)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:22)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:9)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:13)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":124:12)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":125:13)
#loc56 = loc("acir.binding:top_body.0.call.16.103:entry")
#loc57 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":31:12)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":32:13)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":33:13)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:12)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:27)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":127:13)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:16)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:19)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:9)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:9)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":128:9)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:17)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:9)

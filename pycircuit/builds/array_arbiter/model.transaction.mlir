module attributes {acir.model = {config = [], constants = {src_array_arbiter_1242b928.N = 3 : i64}, format_version = 9 : i64, top = "ArrayArbiter", type_order = ["src_array_arbiter_1242b928.Entry"], types = {src_array_arbiter_1242b928.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}, acir.stage = "transaction"} {
  %c2_i64 = arith.constant 2 : i64
  %c1_i64 = arith.constant 1 : i64
  %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
  %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
  %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
  %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
  %c10_i32 = arith.constant {acir.source_type = "u32"} 10 : i32
  %c0_i64 = arith.constant 0 : i64
  %c3_i64 = arith.constant 3 : i64
  %c30_i32 = arith.constant {acir.source_type = "u32"} 30 : i32
  %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
  %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
  %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
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
    %0 = arith.cmpi eq, %value, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc1)
    %1 = arith.cmpi ne, %value, %c0_i32 : i32 loc(#loc1)
    %2 = arith.cmpi eq, %value, %c0_i32 : i32 loc(#loc1)
    %3 = arith.ori %2, %ready : i1
    acir.path %1 {branch = 1 : i64, kind = "branch", txn = -1 : i64} loc(#loc2)
    %value_0, %ready_1 = acir.read(%1) {branch = 1 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc2)
    %4 = arith.cmpi eq, %value_0, %c1_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc2)
    %5 = arith.andi %1, %4 : i1
    %6 = arith.andi %3, %ready_1 : i1
    %7 = arith.cmpi ne, %value_0, %c1_i32 : i32 loc(#loc2)
    %8 = arith.andi %1, %7 : i1
    %9 = arith.xori %8, %true : i1
    %10 = arith.ori %9, %6 : i1
    acir.path %8 {branch = 2 : i64, kind = "branch", txn = -1 : i64} loc(#loc3)
    %value_2, %ready_3 = acir.read(%8) {branch = 2 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc3)
    %11 = arith.cmpi eq, %value_2, %c2_i32 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc3)
    %12 = arith.andi %8, %11 : i1
    %13 = arith.andi %10, %ready_3 : i1
    %14 = arith.xori %12, %true : i1 loc(#loc4)
    %15 = arith.ori %14, %13 : i1
    %16 = arith.andi %12, %15 : i1 loc(#loc4)
    acir.path %12 {branch = 3 : i64, kind = "branch", txn = 0 : i64} loc(#loc4)
    acir.path %12 {branch = 3 : i64, frames = ["top:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc4)
    %17 = arith.ori %14, %15 : i1 loc(#loc4)
    acir.path %12 {branch = 3 : i64, frames = ["top:allocate", "top:$dispatch0"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    %18 = arith.ori %14, %17 : i1 loc(#loc5)
    %19 = arith.andi %17, %16 : i1 loc(#loc5)
    %20 = arith.andi %12, %19 : i1 loc(#loc5)
    %21 = arith.andi %18, %20 : i1
    %22 = arith.ori %14, %21 : i1
    %23 = arith.andi %16, %22 : i1
    %24 = arith.andi %22, %20 : i1
    %25 = arith.andi %12, %24 : i1
    %26 = arith.ori %14, %18 : i1
    %27 = arith.andi %26, %25 : i1
    %28 = arith.ori %14, %27 : i1
    %29 = arith.andi %23, %28 : i1
    %30 = arith.andi %12, %28 : i1
    %31 = arith.ori %14, %26 : i1
    %32 = arith.andi %31, %30 : i1 loc(#loc5)
    %33 = arith.andi %12, %32 : i1 loc(#loc5)
    %34 = arith.andi %17, %33 : i1 loc(#loc5)
    %35 = arith.ori %14, %34 : i1 loc(#loc5)
    %36 = arith.andi %29, %35 : i1 loc(#loc5)
    %37 = arith.andi %34, %16 : i1 loc(#loc5)
    %38 = arith.andi %12, %37 : i1 loc(#loc5)
    %39 = arith.andi %35, %38 : i1
    %40 = arith.ori %14, %39 : i1
    %41 = arith.andi %36, %40 : i1
    %42 = arith.andi %40, %38 : i1
    %43 = arith.ori %14, %42 : i1
    %44 = arith.andi %41, %43 : i1
    acir.path %false {branch = 4 : i64, kind = "branch", txn = 0 : i64} loc(#loc5)
    acir.path %false {branch = 4 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    %45 = acir.aggregate(%true, %true, %c30_i32) {acir.id = "v11", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc6)
    acir.path %12 {branch = 5 : i64, kind = "branch", txn = 0 : i64} loc(#loc5)
    acir.path %12 {branch = 5 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    %46 = arith.ori %14, %43 : i1 loc(#loc5)
    %value_4, %ready_5 = acir.read(%12) {branch = 5 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 0 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc7)
    %47 = arith.andi %46, %ready_5 : i1 loc(#loc7)
    %48 = arith.ori %14, %47 : i1 loc(#loc7)
    %49 = arith.andi %44, %48 : i1 loc(#loc7)
    %50 = acir.extract %value_4 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc7)
    %51 = acir.unary %50 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc8)
    %52 = arith.andi %12, %47 : i1 loc(#loc8)
    %53 = arith.andi %52, %51 : i1 loc(#loc8)
    %54 = arith.andi %43, %53 : i1 loc(#loc8)
    %55 = arith.ori %14, %54 : i1 loc(#loc8)
    %56 = arith.andi %49, %55 : i1 loc(#loc8)
    %57 = arith.andi %54, %38 : i1 loc(#loc8)
    %58 = arith.andi %12, %57 : i1 loc(#loc8)
    %59 = arith.andi %58, %58 : i1 loc(#loc6)
    %60 = arith.andi %55, %59 : i1 loc(#loc9)
    %61 = arith.ori %14, %60 : i1 loc(#loc9)
    %62 = arith.andi %56, %61 : i1 loc(#loc9)
    acir.path %false {branch = 6 : i64, kind = "branch", txn = 0 : i64} loc(#loc5)
    acir.path %false {branch = 6 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc5)
    acir.path %12 {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc10)
    %value_6, %ready_7 = acir.read(%12) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 0 : i64} : (i1) -> (!acir.array<i1, [3]>, i1) loc(#loc11)
    %63 = arith.andi %18, %ready_7 : i1 loc(#loc11)
    %64 = arith.ori %14, %63 : i1 loc(#loc11)
    %65 = arith.andi %62, %64 : i1 loc(#loc11)
    %value_8, %ready_9 = acir.read(%12) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 0 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1) loc(#loc12)
    %66 = arith.andi %18, %ready_9 : i1 loc(#loc12)
    %67 = arith.ori %14, %66 : i1 loc(#loc12)
    %68 = arith.andi %65, %67 : i1 loc(#loc12)
    %69 = "acir.array.index"(%value_6, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %70 = arith.andi %ready_9, %ready_7 : i1 loc(#loc14)
    %71 = arith.andi %70, %20 : i1 loc(#loc14)
    %72 = acir.update %value_8, %69[%c0_i64, %c1_i64] {acir.id = "v12", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %73 = acir.update %72, %false[%c1_i64, %c0_i64] {acir.id = "v15", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %74 = "acir.array.index"(%value_6, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %75 = arith.andi %71, %ready_7 : i1 loc(#loc14)
    %76 = arith.andi %75, %20 : i1 loc(#loc14)
    %77 = acir.update %73, %74[%c1_i64, %c1_i64] {acir.id = "v19", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %78 = acir.update %77, %false[%c1_i64, %c1_i64] {acir.id = "v22", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %79 = "acir.array.index"(%value_6, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %80 = arith.andi %76, %ready_7 : i1 loc(#loc14)
    %81 = arith.andi %80, %20 : i1 loc(#loc14)
    %82 = acir.update %78, %79[%c2_i64, %c1_i64] {acir.id = "v26", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %83 = acir.update %82, %false[%c1_i64, %c2_i64] {acir.id = "v29", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %84 = arith.andi %ready_7, %20 : i1 loc(#loc16)
    %85 = acir.update %value_6, %true[%c1_i64] {acir.id = "v31", acir.source_type = "array<bool;3>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc16)
    %86 = arith.andi %18, %84 : i1 loc(#loc17)
    %87 = arith.ori %14, %86 : i1 loc(#loc17)
    %88 = arith.andi %68, %87 : i1 loc(#loc17)
    %89 = arith.andi %18, %81 : i1 loc(#loc18)
    %90 = arith.ori %14, %89 : i1 loc(#loc18)
    %91 = arith.andi %88, %90 : i1 loc(#loc18)
    %92 = acir.fire "top.allocate#0" %91 {successors = [], txn = 0 : i64}
    acir.effect %false(%45) {branch = 4 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 0 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    %93 = arith.andi %92, %12 : i1
    acir.effect %93(%45) {branch = 5 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 0 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    acir.effect %false(%45) {branch = 6 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 0 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    acir.effect %93(%85) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 0 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc17)
    acir.effect %93(%83) {branch = 3 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.older", txn = 0 : i64} : (i1, !acir.array<i1, [3, 3]>) -> () loc(#loc18)
    %94 = arith.xori %5, %true : i1 loc(#loc19)
    %95 = arith.ori %94, %6 : i1
    %96 = arith.andi %5, %95 : i1 loc(#loc19)
    acir.path %5 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc19)
    acir.path %5 {branch = 7 : i64, frames = ["top:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc19)
    %97 = arith.ori %94, %95 : i1 loc(#loc19)
    acir.path %5 {branch = 7 : i64, frames = ["top:allocate", "top:$dispatch0"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %98 = arith.ori %94, %97 : i1 loc(#loc5)
    %99 = arith.andi %97, %96 : i1 loc(#loc5)
    %100 = arith.andi %5, %99 : i1 loc(#loc5)
    %101 = arith.andi %98, %100 : i1
    %102 = arith.ori %94, %101 : i1
    %103 = arith.andi %96, %102 : i1
    %104 = arith.andi %5, %102 : i1
    %105 = arith.ori %94, %98 : i1
    %106 = arith.andi %105, %104 : i1
    %107 = arith.ori %94, %106 : i1
    %108 = arith.andi %103, %107 : i1
    %109 = arith.andi %5, %107 : i1
    %110 = arith.ori %94, %105 : i1
    %111 = arith.andi %110, %109 : i1 loc(#loc5)
    %112 = arith.andi %5, %111 : i1 loc(#loc5)
    %113 = arith.andi %97, %112 : i1 loc(#loc5)
    %114 = arith.ori %94, %113 : i1 loc(#loc5)
    %115 = arith.andi %108, %114 : i1 loc(#loc5)
    %116 = arith.andi %113, %96 : i1 loc(#loc5)
    %117 = arith.andi %5, %116 : i1 loc(#loc5)
    %118 = arith.andi %114, %117 : i1
    %119 = arith.ori %94, %118 : i1
    %120 = arith.andi %115, %119 : i1
    acir.path %false {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %false {branch = 8 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %121 = acir.aggregate(%true, %true, %c10_i32) {acir.id = "v11", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc6)
    acir.path %false {branch = 9 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %false {branch = 9 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    acir.path %5 {branch = 10 : i64, kind = "branch", txn = 1 : i64} loc(#loc5)
    acir.path %5 {branch = 10 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc5)
    %122 = arith.ori %94, %119 : i1 loc(#loc5)
    %value_10, %ready_11 = acir.read(%5) {branch = 10 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc7)
    %123 = arith.andi %122, %ready_11 : i1 loc(#loc7)
    %124 = arith.ori %94, %123 : i1 loc(#loc7)
    %125 = arith.andi %120, %124 : i1 loc(#loc7)
    %126 = acir.extract %value_10 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc7)
    %127 = acir.unary %126 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc8)
    %128 = arith.andi %5, %123 : i1 loc(#loc8)
    %129 = arith.andi %128, %127 : i1 loc(#loc8)
    %130 = arith.andi %119, %129 : i1 loc(#loc8)
    %131 = arith.ori %94, %130 : i1 loc(#loc8)
    %132 = arith.andi %125, %131 : i1 loc(#loc8)
    %133 = arith.andi %130, %117 : i1 loc(#loc8)
    %134 = arith.andi %5, %133 : i1 loc(#loc8)
    %135 = arith.andi %134, %134 : i1 loc(#loc6)
    %136 = arith.andi %131, %135 : i1 loc(#loc9)
    %137 = arith.ori %94, %136 : i1 loc(#loc9)
    %138 = arith.andi %132, %137 : i1 loc(#loc9)
    acir.path %5 {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc10)
    %value_12, %ready_13 = acir.read(%5) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 1 : i64} : (i1) -> (!acir.array<i1, [3]>, i1) loc(#loc11)
    %139 = arith.andi %98, %ready_13 : i1 loc(#loc11)
    %140 = arith.ori %94, %139 : i1 loc(#loc11)
    %141 = arith.andi %138, %140 : i1 loc(#loc11)
    %value_14, %ready_15 = acir.read(%5) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 1 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1) loc(#loc12)
    %142 = arith.andi %98, %ready_15 : i1 loc(#loc12)
    %143 = arith.ori %94, %142 : i1 loc(#loc12)
    %144 = arith.andi %141, %143 : i1 loc(#loc12)
    %145 = "acir.array.index"(%value_12, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %146 = arith.andi %ready_15, %ready_13 : i1 loc(#loc14)
    %147 = arith.andi %146, %100 : i1 loc(#loc14)
    %148 = acir.update %value_14, %145[%c0_i64, %c0_i64] {acir.id = "v12", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %149 = acir.update %148, %false[%c0_i64, %c0_i64] {acir.id = "v15", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %150 = "acir.array.index"(%value_12, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %151 = arith.andi %147, %ready_13 : i1 loc(#loc14)
    %152 = arith.andi %151, %100 : i1 loc(#loc14)
    %153 = acir.update %149, %150[%c1_i64, %c0_i64] {acir.id = "v19", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %154 = acir.update %153, %false[%c0_i64, %c1_i64] {acir.id = "v22", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %155 = "acir.array.index"(%value_12, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %156 = arith.andi %152, %ready_13 : i1 loc(#loc14)
    %157 = arith.andi %156, %100 : i1 loc(#loc14)
    %158 = acir.update %154, %155[%c2_i64, %c0_i64] {acir.id = "v26", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %159 = acir.update %158, %false[%c0_i64, %c2_i64] {acir.id = "v29", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %160 = arith.andi %ready_13, %100 : i1 loc(#loc16)
    %161 = acir.update %value_12, %true[%c0_i64] {acir.id = "v31", acir.source_type = "array<bool;3>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc16)
    %162 = arith.andi %98, %160 : i1 loc(#loc17)
    %163 = arith.ori %94, %162 : i1 loc(#loc17)
    %164 = arith.andi %144, %163 : i1 loc(#loc17)
    %165 = arith.andi %98, %157 : i1 loc(#loc18)
    %166 = arith.ori %94, %165 : i1 loc(#loc18)
    %167 = arith.andi %164, %166 : i1 loc(#loc18)
    %168 = acir.fire "top.allocate#1" %167 {successors = [], txn = 1 : i64}
    acir.effect %false(%121) {branch = 8 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 1 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    acir.effect %false(%121) {branch = 9 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 1 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    %169 = arith.andi %168, %5 : i1
    acir.effect %169(%121) {branch = 10 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 1 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    acir.effect %169(%161) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 1 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc17)
    acir.effect %169(%159) {branch = 7 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.older", txn = 1 : i64} : (i1, !acir.array<i1, [3, 3]>) -> () loc(#loc18)
    %170 = arith.ori %1, %ready : i1
    %171 = arith.andi %0, %170 : i1 loc(#loc20)
    acir.path %0 {branch = 11 : i64, kind = "branch", txn = 2 : i64} loc(#loc20)
    acir.path %0 {branch = 11 : i64, frames = ["top:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc20)
    %172 = arith.ori %1, %170 : i1 loc(#loc20)
    acir.path %0 {branch = 11 : i64, frames = ["top:allocate", "top:$dispatch0"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    %173 = arith.ori %1, %172 : i1 loc(#loc5)
    %174 = arith.andi %172, %171 : i1 loc(#loc5)
    %175 = arith.andi %0, %174 : i1 loc(#loc5)
    %176 = arith.andi %173, %175 : i1
    %177 = arith.ori %1, %176 : i1
    %178 = arith.andi %171, %177 : i1
    %179 = arith.andi %177, %175 : i1
    %180 = arith.andi %0, %179 : i1
    %181 = arith.ori %1, %173 : i1
    %182 = arith.andi %181, %180 : i1
    %183 = arith.ori %1, %182 : i1
    %184 = arith.andi %178, %183 : i1
    %185 = arith.andi %0, %1 : i1
    %186 = arith.andi %0, %2 : i1
    %187 = arith.xori %186, %true : i1
    %188 = arith.ori %187, %182 : i1
    %189 = arith.xori %185, %true : i1
    %190 = arith.ori %189, %182 : i1
    %191 = arith.andi %188, %175 : i1
    %192 = arith.andi %186, %191 : i1
    %193 = arith.andi %185, %190 : i1
    %194 = arith.ori %192, %193 : i1
    %195 = arith.ori %1, %181 : i1
    %196 = arith.andi %195, %194 : i1 loc(#loc5)
    %197 = arith.andi %0, %196 : i1 loc(#loc5)
    %198 = arith.andi %172, %197 : i1 loc(#loc5)
    %199 = arith.ori %1, %198 : i1 loc(#loc5)
    %200 = arith.andi %184, %199 : i1 loc(#loc5)
    %201 = arith.andi %198, %171 : i1 loc(#loc5)
    %202 = arith.andi %0, %201 : i1 loc(#loc5)
    %203 = arith.andi %199, %202 : i1
    %204 = arith.ori %1, %203 : i1
    %205 = arith.andi %200, %204 : i1
    %206 = arith.andi %204, %202 : i1
    %207 = arith.ori %1, %206 : i1
    %208 = arith.andi %205, %207 : i1
    %209 = arith.andi %207, %202 : i1
    %210 = arith.ori %1, %209 : i1
    %211 = arith.andi %208, %210 : i1
    acir.path %0 {branch = 12 : i64, kind = "branch", txn = 2 : i64} loc(#loc5)
    acir.path %0 {branch = 12 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    %212 = arith.ori %1, %210 : i1 loc(#loc5)
    %value_16, %ready_17 = acir.read(%0) {branch = 12 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 2 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc7)
    %213 = arith.andi %212, %ready_17 : i1 loc(#loc7)
    %214 = arith.ori %1, %213 : i1 loc(#loc7)
    %215 = arith.andi %211, %214 : i1 loc(#loc7)
    %216 = acir.extract %value_16 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc7)
    %217 = acir.unary %216 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc8)
    %218 = arith.andi %0, %213 : i1 loc(#loc8)
    %219 = arith.andi %218, %217 : i1 loc(#loc8)
    %220 = arith.andi %210, %219 : i1 loc(#loc8)
    %221 = arith.ori %1, %220 : i1 loc(#loc8)
    %222 = arith.andi %215, %221 : i1 loc(#loc8)
    %223 = arith.andi %220, %202 : i1 loc(#loc8)
    %224 = arith.andi %0, %223 : i1 loc(#loc8)
    %225 = arith.andi %224, %224 : i1 loc(#loc6)
    %226 = acir.aggregate(%true, %false, %c20_i32) {acir.id = "v11", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc6)
    %227 = arith.andi %221, %225 : i1 loc(#loc9)
    %228 = arith.ori %1, %227 : i1 loc(#loc9)
    %229 = arith.andi %222, %228 : i1 loc(#loc9)
    acir.path %false {branch = 13 : i64, kind = "branch", txn = 2 : i64} loc(#loc5)
    acir.path %false {branch = 13 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    acir.path %false {branch = 14 : i64, kind = "branch", txn = 2 : i64} loc(#loc5)
    acir.path %false {branch = 14 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc5)
    acir.path %0 {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc10)
    %value_18, %ready_19 = acir.read(%0) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 2 : i64} : (i1) -> (!acir.array<i1, [3]>, i1) loc(#loc11)
    %230 = arith.andi %173, %ready_19 : i1 loc(#loc11)
    %231 = arith.ori %1, %230 : i1 loc(#loc11)
    %232 = arith.andi %229, %231 : i1 loc(#loc11)
    %value_20, %ready_21 = acir.read(%0) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 2 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1) loc(#loc12)
    %233 = arith.andi %173, %ready_21 : i1 loc(#loc12)
    %234 = arith.ori %1, %233 : i1 loc(#loc12)
    %235 = arith.andi %232, %234 : i1 loc(#loc12)
    %236 = "acir.array.index"(%value_18, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %237 = arith.andi %ready_21, %ready_19 : i1 loc(#loc14)
    %238 = arith.andi %237, %175 : i1 loc(#loc14)
    %239 = acir.update %value_20, %236[%c0_i64, %c2_i64] {acir.id = "v12", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %240 = acir.update %239, %false[%c2_i64, %c0_i64] {acir.id = "v15", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %241 = "acir.array.index"(%value_18, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %242 = arith.andi %238, %ready_19 : i1 loc(#loc14)
    %243 = arith.andi %242, %175 : i1 loc(#loc14)
    %244 = acir.update %240, %241[%c1_i64, %c2_i64] {acir.id = "v19", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %245 = acir.update %244, %false[%c2_i64, %c1_i64] {acir.id = "v22", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %246 = "acir.array.index"(%value_18, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc13)
    %247 = arith.andi %243, %ready_19 : i1 loc(#loc14)
    %248 = arith.andi %247, %175 : i1 loc(#loc14)
    %249 = acir.update %245, %246[%c2_i64, %c2_i64] {acir.id = "v26", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc14)
    %250 = acir.update %249, %false[%c2_i64, %c2_i64] {acir.id = "v29", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i64, i64) -> !acir.array<i1, [3, 3]> loc(#loc15)
    %251 = arith.andi %ready_19, %175 : i1 loc(#loc16)
    %252 = acir.update %value_18, %true[%c2_i64] {acir.id = "v31", acir.source_type = "array<bool;3>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc16)
    %253 = arith.andi %173, %251 : i1 loc(#loc17)
    %254 = arith.ori %1, %253 : i1 loc(#loc17)
    %255 = arith.andi %235, %254 : i1 loc(#loc17)
    %256 = arith.andi %173, %248 : i1 loc(#loc18)
    %257 = arith.ori %1, %256 : i1 loc(#loc18)
    %258 = arith.andi %255, %257 : i1 loc(#loc18)
    %259 = acir.fire "top.allocate#2" %258 {successors = [], txn = 2 : i64}
    %260 = arith.andi %259, %0 : i1
    acir.effect %260(%226) {branch = 12 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.2:allocate"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 2 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    acir.effect %false(%226) {branch = 13 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.1:allocate"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 2 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    acir.effect %false(%226) {branch = 14 : i64, frames = ["top:allocate", "top:$dispatch0", "top.slots.0:allocate"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 2 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc9)
    acir.effect %260(%252) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 2 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc17)
    acir.effect %260(%250) {branch = 11 : i64, frames = ["top:allocate", "top.age:allocate"], method = "write", port = 0 : i64, resource = "top.age.older", txn = 2 : i64} : (i1, !acir.array<i1, [3, 3]>) -> () loc(#loc18)
    acir.path %true {branch = 0 : i64, frames = ["top.slots.0:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc21)
    %value_22, %ready_23 = acir.read(%true) {branch = 0 : i64, frames = ["top.slots.0:peek"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc22)
    %261 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    %262 = acir.assert %261
    %263 = arith.andi %ready_23, %262 : i1
    acir.path %true {branch = 0 : i64, frames = ["top.slots.1:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc21)
    %value_24, %ready_25 = acir.read(%true) {branch = 0 : i64, frames = ["top.slots.1:peek"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc22)
    %264 = acir.assert %261
    %265 = arith.andi %ready_25, %264 : i1
    acir.path %true {branch = 0 : i64, frames = ["top.slots.2:peek"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc21)
    %value_26, %ready_27 = acir.read(%true) {branch = 0 : i64, frames = ["top.slots.2:peek"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = -1 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc22)
    %266 = acir.assert %261
    %267 = arith.andi %ready_27, %266 : i1
    %268 = arith.andi %263, %265 : i1 loc(#loc23)
    %269 = arith.andi %268, %267 : i1 loc(#loc23)
    %270 = acir.aggregate(%value_22, %value_24, %value_26) {acir.id = "v44", acir.source_type = "array<src_array_arbiter_1242b928.Entry;3>"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> !acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]> loc(#loc23)
    %value_28, %ready_29 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc24)
    %271 = arith.cmpi uge, %value_28, %c3_i32 {acir.id = "v48", acir.source_type = "bool"} : i32 loc(#loc24)
    %272 = arith.cmpi ult, %value_28, %c3_i32 : i32 loc(#loc24)
    %273 = arith.ori %272, %ready_29 : i1
    %274 = "acir.array.domain"(%270) {acir.id = "v49", acir.source_type = "mask<3>"} : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>) -> !acir.mask<[3]> loc(#loc25)
    %275 = "acir.array.index"(%270, %c0_i64) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i64) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc25)
    %276 = "acir.array.index"(%274, %c0_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc25)
    %277 = arith.andi %271, %276 : i1
    %278 = arith.andi %273, %269 : i1
    %279 = arith.xori %276, %true : i1
    %280 = arith.andi %271, %279 : i1
    %281 = arith.xori %280, %true : i1
    %282 = arith.ori %281, %278 : i1
    %283 = arith.xori %277, %true : i1
    %284 = arith.ori %283, %278 : i1
    %285 = acir.extract %275 {acir.id = "v54", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc26)
    %286 = arith.andi %277, %285 : i1
    %287 = arith.andi %284, %269 : i1
    %288 = arith.xori %285, %true : i1
    %289 = arith.andi %277, %288 : i1
    %290 = arith.xori %289, %true : i1
    %291 = arith.ori %290, %287 : i1
    %292 = arith.xori %286, %true : i1
    %293 = arith.ori %292, %287 : i1
    %294 = acir.extract %275 {acir.id = "v55", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc27)
    %295 = arith.andi %289, %291 : i1
    %296 = arith.andi %293, %269 : i1
    %297 = arith.andi %286, %296 : i1
    %298 = arith.ori %295, %297 : i1
    %299 = arith.select %289, %false, %294 : i1
    %300 = arith.ori %283, %284 : i1
    %301 = arith.andi %280, %282 : i1
    %302 = arith.andi %300, %298 : i1
    %303 = arith.andi %277, %302 : i1
    %304 = arith.ori %301, %303 : i1
    %305 = arith.select %280, %false, %299 : i1
    %306 = arith.ori %272, %273 : i1
    %307 = "acir.array.index"(%270, %c1_i64) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i64) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc25)
    %308 = "acir.array.index"(%274, %c1_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc25)
    %309 = arith.andi %271, %308 : i1
    %310 = arith.andi %306, %269 : i1
    %311 = arith.xori %308, %true : i1
    %312 = arith.andi %271, %311 : i1
    %313 = arith.xori %312, %true : i1
    %314 = arith.ori %313, %310 : i1
    %315 = arith.xori %309, %true : i1
    %316 = arith.ori %315, %310 : i1
    %317 = acir.extract %307 {acir.id = "v68", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc26)
    %318 = arith.andi %309, %317 : i1
    %319 = arith.andi %316, %269 : i1
    %320 = arith.xori %317, %true : i1
    %321 = arith.andi %309, %320 : i1
    %322 = arith.xori %321, %true : i1
    %323 = arith.ori %322, %319 : i1
    %324 = arith.xori %318, %true : i1
    %325 = arith.ori %324, %319 : i1
    %326 = acir.extract %307 {acir.id = "v69", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc27)
    %327 = arith.andi %321, %323 : i1
    %328 = arith.andi %325, %269 : i1
    %329 = arith.andi %318, %328 : i1
    %330 = arith.ori %327, %329 : i1
    %331 = arith.select %321, %false, %326 : i1
    %332 = arith.ori %315, %316 : i1
    %333 = arith.andi %312, %314 : i1
    %334 = arith.andi %332, %330 : i1
    %335 = arith.andi %309, %334 : i1
    %336 = arith.ori %333, %335 : i1
    %337 = arith.select %312, %false, %331 : i1
    %338 = arith.ori %272, %306 : i1
    %339 = "acir.array.index"(%270, %c2_i64) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i64) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc25)
    %340 = "acir.array.index"(%274, %c2_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc25)
    %341 = arith.andi %271, %340 : i1
    %342 = arith.andi %338, %269 : i1
    %343 = arith.xori %340, %true : i1
    %344 = arith.andi %271, %343 : i1
    %345 = arith.xori %344, %true : i1
    %346 = arith.ori %345, %342 : i1
    %347 = arith.xori %341, %true : i1
    %348 = arith.ori %347, %342 : i1
    %349 = acir.extract %339 {acir.id = "v82", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc26)
    %350 = arith.andi %341, %349 : i1
    %351 = arith.andi %348, %269 : i1
    %352 = arith.xori %349, %true : i1
    %353 = arith.andi %341, %352 : i1
    %354 = arith.xori %353, %true : i1
    %355 = arith.ori %354, %351 : i1
    %356 = arith.xori %350, %true : i1
    %357 = arith.ori %356, %351 : i1
    %358 = acir.extract %339 {acir.id = "v83", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc27)
    %359 = arith.andi %353, %355 : i1
    %360 = arith.andi %357, %269 : i1
    %361 = arith.andi %350, %360 : i1
    %362 = arith.ori %359, %361 : i1
    %363 = arith.select %353, %false, %358 : i1
    %364 = arith.ori %347, %348 : i1
    %365 = arith.andi %344, %346 : i1
    %366 = arith.andi %364, %362 : i1
    %367 = arith.andi %341, %366 : i1
    %368 = arith.ori %365, %367 : i1
    %369 = arith.select %344, %false, %363 : i1
    %370 = arith.ori %272, %338 : i1
    %371 = arith.andi %304, %336 : i1 loc(#loc25)
    %372 = arith.andi %371, %368 : i1 loc(#loc25)
    %373 = acir.aggregate(%305, %337, %369) {acir.id = "v92", acir.source_type = "mask<3>"} : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc25)
    %374 = arith.andi %269, %372 : i1 loc(#loc28)
    %375 = "acir.array.select"(%270, %373) {acir.id = "v93", acir.source_type = "selected<array<src_array_arbiter_1242b928.Entry;3>>"} : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, !acir.mask<[3]>) -> !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>> loc(#loc28)
    %376 = arith.andi %370, %374 : i1 loc(#loc29)
    %377 = arith.andi %271, %376 : i1 loc(#loc29)
    acir.path %271 {branch = 15 : i64, kind = "branch", txn = 3 : i64} loc(#loc29)
    acir.path %271 {branch = 15 : i64, frames = ["top:issue"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc29)
    %378 = arith.ori %272, %370 : i1 loc(#loc29)
    acir.path %271 {branch = 15 : i64, frames = ["top:issue", "top.age:choose"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc30)
    %379 = arith.ori %272, %378 : i1 loc(#loc30)
    %380 = arith.andi %378, %377 : i1 loc(#loc30)
    %381 = arith.andi %271, %380 : i1 loc(#loc30)
    %382 = "acir.array.domain"(%375) {acir.id = "v7", acir.source_type = "mask<3>"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> !acir.mask<[3]> loc(#loc31)
    %383 = "acir.array.index"(%382, %c0_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc31)
    %384 = arith.andi %379, %381 : i1
    %385 = arith.ori %272, %384 : i1
    %386 = arith.andi %377, %385 : i1
    %387 = arith.andi %271, %383 : i1
    %388 = arith.xori %383, %true : i1
    %389 = arith.andi %271, %388 : i1
    %390 = arith.xori %389, %true : i1
    %391 = arith.ori %390, %384 : i1
    %392 = arith.xori %387, %true : i1
    %393 = arith.ori %392, %384 : i1
    %394 = arith.andi %389, %391 : i1
    %395 = arith.andi %387, %393 : i1
    %396 = arith.ori %394, %395 : i1
    %397 = arith.ori %272, %379 : i1
    %398 = "acir.array.index"(%382, %c1_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc31)
    %399 = arith.andi %397, %381 : i1
    %400 = arith.ori %272, %399 : i1
    %401 = arith.andi %386, %400 : i1
    %402 = arith.andi %271, %398 : i1
    %403 = arith.xori %398, %true : i1
    %404 = arith.andi %271, %403 : i1
    %405 = arith.xori %404, %true : i1
    %406 = arith.ori %405, %399 : i1
    %407 = arith.xori %402, %true : i1
    %408 = arith.ori %407, %399 : i1
    %409 = arith.andi %404, %406 : i1
    %410 = arith.andi %402, %408 : i1
    %411 = arith.ori %409, %410 : i1
    %412 = arith.ori %272, %397 : i1
    %413 = "acir.array.index"(%382, %c2_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc31)
    %414 = arith.andi %412, %381 : i1
    %415 = arith.ori %272, %414 : i1
    %416 = arith.andi %401, %415 : i1
    %417 = arith.andi %271, %413 : i1
    %418 = arith.xori %413, %true : i1
    %419 = arith.andi %271, %418 : i1
    %420 = arith.xori %419, %true : i1
    %421 = arith.ori %420, %414 : i1
    %422 = arith.xori %417, %true : i1
    %423 = arith.ori %422, %414 : i1
    %424 = arith.andi %419, %421 : i1
    %425 = arith.andi %417, %423 : i1
    %426 = arith.ori %424, %425 : i1
    %427 = arith.ori %272, %412 : i1
    %428 = arith.andi %396, %411 : i1 loc(#loc31)
    %429 = arith.andi %428, %426 : i1 loc(#loc31)
    %430 = acir.aggregate(%390, %405, %420) {acir.id = "v35", acir.source_type = "mask<3>"} : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc31)
    %value_30, %ready_31 = acir.read(%271) {branch = 15 : i64, frames = ["top:issue", "top.age:choose"], method = "read", port = 0 : i64, resource = "top.age.older", txn = 3 : i64} : (i1) -> (!acir.array<i1, [3, 3]>, i1) loc(#loc32)
    %431 = arith.andi %427, %ready_31 : i1 loc(#loc32)
    %432 = arith.ori %272, %431 : i1 loc(#loc32)
    %433 = arith.andi %416, %432 : i1 loc(#loc32)
    %434 = "acir.array.index"(%430, %c0_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc33)
    %435 = arith.andi %427, %429 : i1
    %436 = arith.ori %272, %435 : i1
    %437 = arith.andi %433, %436 : i1
    %438 = arith.andi %271, %434 : i1
    %439 = arith.xori %434, %true : i1
    %440 = arith.andi %271, %439 : i1
    %441 = arith.xori %440, %true : i1
    %442 = arith.ori %441, %435 : i1
    %443 = arith.xori %438, %true : i1
    %444 = arith.ori %443, %435 : i1
    %445 = "acir.array.index"(%value_30, %c0_i64) : (!acir.array<i1, [3, 3]>, i64) -> !acir.array<i1, [3]> loc(#loc34)
    %446 = "acir.array.index"(%445, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %447 = arith.andi %440, %442 : i1
    %448 = arith.andi %444, %ready_31 : i1
    %449 = arith.andi %438, %448 : i1
    %450 = arith.ori %447, %449 : i1
    %451 = arith.select %440, %false, %446 : i1
    %452 = arith.ori %272, %427 : i1
    %453 = arith.andi %452, %450 : i1
    %454 = arith.ori %272, %453 : i1
    %455 = arith.andi %437, %454 : i1
    %456 = arith.andi %271, %451 : i1
    %457 = arith.xori %451, %true : i1
    %458 = arith.andi %271, %457 : i1
    %459 = arith.xori %458, %true : i1
    %460 = arith.ori %459, %453 : i1
    %461 = arith.xori %456, %true : i1
    %462 = arith.ori %461, %453 : i1
    %463 = arith.andi %458, %460 : i1
    %464 = arith.andi %456, %462 : i1
    %465 = arith.ori %463, %464 : i1
    %466 = arith.ori %272, %452 : i1
    %467 = "acir.array.index"(%430, %c1_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc33)
    %468 = arith.andi %466, %429 : i1
    %469 = arith.ori %272, %468 : i1
    %470 = arith.andi %455, %469 : i1
    %471 = arith.andi %271, %467 : i1
    %472 = arith.xori %467, %true : i1
    %473 = arith.andi %271, %472 : i1
    %474 = arith.xori %473, %true : i1
    %475 = arith.ori %474, %468 : i1
    %476 = arith.xori %471, %true : i1
    %477 = arith.ori %476, %468 : i1
    %478 = "acir.array.index"(%value_30, %c1_i64) : (!acir.array<i1, [3, 3]>, i64) -> !acir.array<i1, [3]> loc(#loc34)
    %479 = "acir.array.index"(%478, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %480 = arith.andi %473, %475 : i1
    %481 = arith.andi %477, %ready_31 : i1
    %482 = arith.andi %471, %481 : i1
    %483 = arith.ori %480, %482 : i1
    %484 = arith.select %473, %false, %479 : i1
    %485 = arith.ori %272, %466 : i1
    %486 = arith.andi %485, %483 : i1
    %487 = arith.ori %272, %486 : i1
    %488 = arith.andi %470, %487 : i1
    %489 = arith.andi %271, %484 : i1
    %490 = arith.xori %484, %true : i1
    %491 = arith.andi %271, %490 : i1
    %492 = arith.xori %491, %true : i1
    %493 = arith.ori %492, %486 : i1
    %494 = arith.xori %489, %true : i1
    %495 = arith.ori %494, %486 : i1
    %496 = arith.andi %493, %465 : i1
    %497 = arith.andi %491, %496 : i1
    %498 = arith.andi %489, %495 : i1
    %499 = arith.ori %497, %498 : i1
    %500 = arith.select %491, %458, %false : i1
    %501 = arith.ori %272, %485 : i1
    %502 = "acir.array.index"(%430, %c2_i64) : (!acir.mask<[3]>, i64) -> i1 loc(#loc33)
    %503 = arith.andi %501, %429 : i1
    %504 = arith.ori %272, %503 : i1
    %505 = arith.andi %488, %504 : i1
    %506 = arith.andi %271, %502 : i1
    %507 = arith.xori %502, %true : i1
    %508 = arith.andi %271, %507 : i1
    %509 = arith.xori %508, %true : i1
    %510 = arith.ori %509, %503 : i1
    %511 = arith.xori %506, %true : i1
    %512 = arith.ori %511, %503 : i1
    %513 = "acir.array.index"(%value_30, %c2_i64) : (!acir.array<i1, [3, 3]>, i64) -> !acir.array<i1, [3]> loc(#loc34)
    %514 = "acir.array.index"(%513, %c0_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %515 = arith.andi %508, %510 : i1
    %516 = arith.andi %512, %ready_31 : i1
    %517 = arith.andi %506, %516 : i1
    %518 = arith.ori %515, %517 : i1
    %519 = arith.select %508, %false, %514 : i1
    %520 = arith.ori %272, %501 : i1
    %521 = arith.andi %520, %518 : i1
    %522 = arith.ori %272, %521 : i1
    %523 = arith.andi %505, %522 : i1
    %524 = arith.andi %271, %519 : i1
    %525 = arith.xori %519, %true : i1
    %526 = arith.andi %271, %525 : i1
    %527 = arith.xori %526, %true : i1
    %528 = arith.ori %527, %521 : i1
    %529 = arith.xori %524, %true : i1
    %530 = arith.ori %529, %521 : i1
    %531 = arith.andi %528, %499 : i1
    %532 = arith.andi %526, %531 : i1
    %533 = arith.andi %524, %530 : i1
    %534 = arith.ori %532, %533 : i1
    %535 = arith.select %526, %500, %false : i1
    %536 = arith.ori %272, %520 : i1
    %537 = arith.andi %536, %429 : i1
    %538 = arith.ori %272, %537 : i1
    %539 = arith.andi %523, %538 : i1
    %540 = arith.ori %441, %537 : i1
    %541 = arith.ori %443, %537 : i1
    %542 = arith.andi %440, %540 : i1
    %543 = arith.andi %541, %534 : i1
    %544 = arith.andi %438, %543 : i1
    %545 = arith.ori %542, %544 : i1
    %546 = arith.select %440, %false, %535 : i1
    %547 = arith.ori %272, %536 : i1
    %548 = arith.andi %547, %545 : i1
    %549 = arith.ori %272, %548 : i1
    %550 = arith.andi %539, %549 : i1
    %551 = arith.andi %271, %546 : i1
    %552 = arith.xori %546, %true : i1
    %553 = arith.andi %271, %552 : i1
    %554 = arith.xori %553, %true : i1
    %555 = arith.ori %554, %548 : i1
    %556 = arith.xori %551, %true : i1
    %557 = arith.ori %556, %548 : i1
    %558 = arith.andi %553, %555 : i1
    %559 = arith.andi %551, %557 : i1
    %560 = arith.ori %558, %559 : i1
    %561 = arith.ori %272, %547 : i1
    %562 = arith.andi %561, %429 : i1
    %563 = arith.ori %272, %562 : i1
    %564 = arith.andi %550, %563 : i1
    %565 = arith.ori %441, %562 : i1
    %566 = arith.ori %443, %562 : i1
    %567 = "acir.array.index"(%445, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %568 = arith.andi %440, %565 : i1
    %569 = arith.andi %566, %ready_31 : i1
    %570 = arith.andi %438, %569 : i1
    %571 = arith.ori %568, %570 : i1
    %572 = arith.select %440, %false, %567 : i1
    %573 = arith.ori %272, %561 : i1
    %574 = arith.andi %573, %571 : i1
    %575 = arith.ori %272, %574 : i1
    %576 = arith.andi %564, %575 : i1
    %577 = arith.andi %271, %572 : i1
    %578 = arith.xori %572, %true : i1
    %579 = arith.andi %271, %578 : i1
    %580 = arith.xori %579, %true : i1
    %581 = arith.ori %580, %574 : i1
    %582 = arith.xori %577, %true : i1
    %583 = arith.ori %582, %574 : i1
    %584 = arith.andi %579, %581 : i1
    %585 = arith.andi %577, %583 : i1
    %586 = arith.ori %584, %585 : i1
    %587 = arith.ori %272, %573 : i1
    %588 = arith.andi %587, %429 : i1
    %589 = arith.ori %272, %588 : i1
    %590 = arith.andi %576, %589 : i1
    %591 = arith.ori %474, %588 : i1
    %592 = arith.ori %476, %588 : i1
    %593 = "acir.array.index"(%478, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %594 = arith.andi %473, %591 : i1
    %595 = arith.andi %592, %ready_31 : i1
    %596 = arith.andi %471, %595 : i1
    %597 = arith.ori %594, %596 : i1
    %598 = arith.select %473, %false, %593 : i1
    %599 = arith.ori %272, %587 : i1
    %600 = arith.andi %599, %597 : i1
    %601 = arith.ori %272, %600 : i1
    %602 = arith.andi %590, %601 : i1
    %603 = arith.andi %271, %598 : i1
    %604 = arith.xori %598, %true : i1
    %605 = arith.andi %271, %604 : i1
    %606 = arith.xori %605, %true : i1
    %607 = arith.ori %606, %600 : i1
    %608 = arith.xori %603, %true : i1
    %609 = arith.ori %608, %600 : i1
    %610 = arith.andi %607, %586 : i1
    %611 = arith.andi %605, %610 : i1
    %612 = arith.andi %603, %609 : i1
    %613 = arith.ori %611, %612 : i1
    %614 = arith.select %605, %579, %false : i1
    %615 = arith.ori %272, %599 : i1
    %616 = arith.andi %615, %429 : i1
    %617 = arith.ori %272, %616 : i1
    %618 = arith.andi %602, %617 : i1
    %619 = arith.ori %509, %616 : i1
    %620 = arith.ori %511, %616 : i1
    %621 = "acir.array.index"(%513, %c1_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %622 = arith.andi %508, %619 : i1
    %623 = arith.andi %620, %ready_31 : i1
    %624 = arith.andi %506, %623 : i1
    %625 = arith.ori %622, %624 : i1
    %626 = arith.select %508, %false, %621 : i1
    %627 = arith.ori %272, %615 : i1
    %628 = arith.andi %627, %625 : i1
    %629 = arith.ori %272, %628 : i1
    %630 = arith.andi %618, %629 : i1
    %631 = arith.andi %271, %626 : i1
    %632 = arith.xori %626, %true : i1
    %633 = arith.andi %271, %632 : i1
    %634 = arith.xori %633, %true : i1
    %635 = arith.ori %634, %628 : i1
    %636 = arith.xori %631, %true : i1
    %637 = arith.ori %636, %628 : i1
    %638 = arith.andi %635, %613 : i1
    %639 = arith.andi %633, %638 : i1
    %640 = arith.andi %631, %637 : i1
    %641 = arith.ori %639, %640 : i1
    %642 = arith.select %633, %614, %false : i1
    %643 = arith.ori %272, %627 : i1
    %644 = arith.andi %643, %429 : i1
    %645 = arith.ori %272, %644 : i1
    %646 = arith.andi %630, %645 : i1
    %647 = arith.ori %474, %644 : i1
    %648 = arith.ori %476, %644 : i1
    %649 = arith.andi %473, %647 : i1
    %650 = arith.andi %648, %641 : i1
    %651 = arith.andi %471, %650 : i1
    %652 = arith.ori %649, %651 : i1
    %653 = arith.select %473, %false, %642 : i1
    %654 = arith.ori %272, %643 : i1
    %655 = arith.andi %654, %652 : i1
    %656 = arith.ori %272, %655 : i1
    %657 = arith.andi %646, %656 : i1
    %658 = arith.andi %271, %653 : i1
    %659 = arith.xori %653, %true : i1
    %660 = arith.andi %271, %659 : i1
    %661 = arith.xori %660, %true : i1
    %662 = arith.ori %661, %655 : i1
    %663 = arith.xori %658, %true : i1
    %664 = arith.ori %663, %655 : i1
    %665 = arith.andi %662, %560 : i1
    %666 = arith.andi %660, %665 : i1
    %667 = arith.andi %658, %664 : i1
    %668 = arith.ori %666, %667 : i1
    %669 = arith.select %660, %554, %true : i1
    %670 = arith.extui %661 : i1 to i32
    %671 = arith.ori %272, %654 : i1
    %672 = arith.andi %671, %429 : i1
    %673 = arith.ori %272, %672 : i1
    %674 = arith.andi %657, %673 : i1
    %675 = arith.ori %441, %672 : i1
    %676 = arith.ori %443, %672 : i1
    %677 = "acir.array.index"(%445, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %678 = arith.andi %440, %675 : i1
    %679 = arith.andi %676, %ready_31 : i1
    %680 = arith.andi %438, %679 : i1
    %681 = arith.ori %678, %680 : i1
    %682 = arith.select %440, %false, %677 : i1
    %683 = arith.ori %272, %671 : i1
    %684 = arith.andi %683, %681 : i1
    %685 = arith.ori %272, %684 : i1
    %686 = arith.andi %674, %685 : i1
    %687 = arith.andi %271, %682 : i1
    %688 = arith.xori %682, %true : i1
    %689 = arith.andi %271, %688 : i1
    %690 = arith.xori %689, %true : i1
    %691 = arith.ori %690, %684 : i1
    %692 = arith.xori %687, %true : i1
    %693 = arith.ori %692, %684 : i1
    %694 = arith.andi %689, %691 : i1
    %695 = arith.andi %687, %693 : i1
    %696 = arith.ori %694, %695 : i1
    %697 = arith.ori %272, %683 : i1
    %698 = arith.andi %697, %429 : i1
    %699 = arith.ori %272, %698 : i1
    %700 = arith.andi %686, %699 : i1
    %701 = arith.ori %474, %698 : i1
    %702 = arith.ori %476, %698 : i1
    %703 = "acir.array.index"(%478, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %704 = arith.andi %473, %701 : i1
    %705 = arith.andi %702, %ready_31 : i1
    %706 = arith.andi %471, %705 : i1
    %707 = arith.ori %704, %706 : i1
    %708 = arith.select %473, %false, %703 : i1
    %709 = arith.ori %272, %697 : i1
    %710 = arith.andi %709, %707 : i1
    %711 = arith.ori %272, %710 : i1
    %712 = arith.andi %700, %711 : i1
    %713 = arith.andi %271, %708 : i1
    %714 = arith.xori %708, %true : i1
    %715 = arith.andi %271, %714 : i1
    %716 = arith.xori %715, %true : i1
    %717 = arith.ori %716, %710 : i1
    %718 = arith.xori %713, %true : i1
    %719 = arith.ori %718, %710 : i1
    %720 = arith.andi %717, %696 : i1
    %721 = arith.andi %715, %720 : i1
    %722 = arith.andi %713, %719 : i1
    %723 = arith.ori %721, %722 : i1
    %724 = arith.select %715, %689, %false : i1
    %725 = arith.ori %272, %709 : i1
    %726 = arith.andi %725, %429 : i1
    %727 = arith.ori %272, %726 : i1
    %728 = arith.andi %712, %727 : i1
    %729 = arith.ori %509, %726 : i1
    %730 = arith.ori %511, %726 : i1
    %731 = "acir.array.index"(%513, %c2_i64) : (!acir.array<i1, [3]>, i64) -> i1 loc(#loc34)
    %732 = arith.andi %508, %729 : i1
    %733 = arith.andi %730, %ready_31 : i1
    %734 = arith.andi %506, %733 : i1
    %735 = arith.ori %732, %734 : i1
    %736 = arith.select %508, %false, %731 : i1
    %737 = arith.ori %272, %725 : i1
    %738 = arith.andi %737, %735 : i1
    %739 = arith.ori %272, %738 : i1
    %740 = arith.andi %728, %739 : i1
    %741 = arith.andi %271, %736 : i1
    %742 = arith.xori %736, %true : i1
    %743 = arith.andi %271, %742 : i1
    %744 = arith.xori %743, %true : i1
    %745 = arith.ori %744, %738 : i1
    %746 = arith.xori %741, %true : i1
    %747 = arith.ori %746, %738 : i1
    %748 = arith.andi %745, %723 : i1
    %749 = arith.andi %743, %748 : i1
    %750 = arith.andi %741, %747 : i1
    %751 = arith.ori %749, %750 : i1
    %752 = arith.select %743, %724, %false : i1
    %753 = arith.ori %272, %737 : i1
    %754 = arith.andi %753, %429 : i1
    %755 = arith.ori %272, %754 : i1
    %756 = arith.andi %740, %755 : i1
    %757 = arith.ori %509, %754 : i1
    %758 = arith.ori %511, %754 : i1
    %759 = arith.andi %508, %757 : i1
    %760 = arith.andi %758, %751 : i1
    %761 = arith.andi %506, %760 : i1
    %762 = arith.ori %759, %761 : i1
    %763 = arith.select %508, %false, %752 : i1
    %764 = arith.ori %272, %753 : i1
    %765 = arith.andi %764, %762 : i1
    %766 = arith.ori %272, %765 : i1
    %767 = arith.andi %756, %766 : i1
    %768 = arith.andi %271, %763 : i1
    %769 = arith.xori %763, %true : i1
    %770 = arith.andi %271, %769 : i1
    %771 = arith.xori %770, %true : i1
    %772 = arith.ori %771, %765 : i1
    %773 = arith.xori %768, %true : i1
    %774 = arith.ori %773, %765 : i1
    %775 = arith.andi %772, %668 : i1
    %776 = arith.andi %770, %775 : i1
    %777 = arith.andi %768, %774 : i1
    %778 = arith.ori %776, %777 : i1
    %779 = arith.select %770, %669, %true : i1
    %780 = arith.select %770, %670, %c2_i32 : i32
    %781 = arith.ori %272, %764 : i1
    %782 = arith.andi %778, %778 : i1 loc(#loc35)
    %783 = acir.aggregate(%779, %780) {acir.id = "v219", acir.source_type = "tuple<bool,u32>"} : (i1, i32) -> tuple<i1, i32> loc(#loc35)
    %784 = arith.ori %272, %781 : i1
    %785 = arith.andi %767, %784 : i1
    %786 = arith.andi %781, %782 : i1
    %787 = arith.andi %271, %786 : i1
    %788 = acir.extract %783[%c0_i32] {acir.id = "v7", acir.source_type = "bool", indexed = true, output_call = "v5", slot = 0 : i64} : (tuple<i1, i32>, i32) -> i1 loc(#loc30)
    %789 = acir.extract %783[%c1_i32] {acir.id = "v9", acir.source_type = "u32", indexed = true, output_call = "v5", slot = 1 : i64} : (tuple<i1, i32>, i32) -> i32 loc(#loc30)
    %790 = arith.andi %378, %787 : i1
    %791 = arith.ori %272, %790 : i1
    %792 = arith.andi %785, %791 : i1
    %793 = arith.andi %271, %788 : i1
    %794 = arith.xori %793, %true : i1 loc(#loc36)
    %795 = arith.ori %794, %790 : i1
    %796 = arith.cmpi eq, %789, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc36)
    %797 = arith.cmpi eq, %789, %c1_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc36)
    %798 = arith.cmpi eq, %789, %c2_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc36)
    acir.path %793 {branch = 16 : i64, kind = "branch", txn = 3 : i64} loc(#loc36)
    acir.path %793 {branch = 16 : i64, frames = ["top:issue", "top:$dispatch10"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc36)
    %799 = arith.ori %794, %795 : i1 loc(#loc36)
    %800 = arith.andi %795, %787 : i1 loc(#loc36)
    %801 = arith.andi %793, %800 : i1 loc(#loc36)
    %802 = arith.andi %799, %801 : i1
    %803 = arith.ori %794, %802 : i1
    %804 = arith.andi %792, %803 : i1
    %805 = arith.andi %793, %796 : i1
    %806 = arith.cmpi ne, %789, %c0_i32 : i32 loc(#loc36)
    %807 = arith.andi %793, %806 : i1
    %808 = arith.xori %807, %true : i1
    %809 = arith.ori %808, %802 : i1
    %810 = arith.xori %805, %true : i1 loc(#loc36)
    %811 = arith.ori %810, %802 : i1
    %812 = arith.andi %809, %801 : i1
    %813 = arith.andi %807, %812 : i1
    %814 = arith.andi %805, %811 : i1
    %815 = arith.ori %813, %814 : i1
    %816 = arith.select %807, %797, %true : i1
    %817 = arith.ori %794, %799 : i1
    %818 = arith.andi %817, %815 : i1
    %819 = arith.ori %794, %818 : i1
    %820 = arith.andi %804, %819 : i1
    %821 = arith.andi %793, %816 : i1
    %822 = arith.xori %816, %true : i1
    %823 = arith.andi %793, %822 : i1
    %824 = arith.xori %823, %true : i1
    %825 = arith.ori %824, %818 : i1
    %826 = arith.xori %821, %true : i1
    %827 = arith.ori %826, %818 : i1
    %828 = arith.andi %825, %801 : i1
    %829 = arith.andi %823, %828 : i1
    %830 = arith.andi %821, %827 : i1
    %831 = arith.ori %829, %830 : i1
    %832 = arith.select %823, %798, %true : i1
    %833 = arith.ori %794, %817 : i1
    %834 = arith.andi %833, %831 : i1 loc(#loc36)
    %835 = arith.andi %793, %834 : i1 loc(#loc36)
    %836 = arith.andi %835, %832 : i1 loc(#loc36)
    %837 = arith.andi %795, %836 : i1 loc(#loc36)
    %838 = arith.ori %794, %837 : i1 loc(#loc36)
    %839 = arith.andi %820, %838 : i1 loc(#loc36)
    %840 = arith.andi %837, %787 : i1 loc(#loc36)
    %841 = arith.andi %793, %840 : i1 loc(#loc36)
    %842 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
    %843 = arith.andi %838, %841 : i1
    %844 = arith.ori %794, %843 : i1
    %845 = arith.andi %839, %844 : i1
    %846 = arith.ori %808, %843 : i1
    %847 = arith.andi %846, %841 : i1
    %848 = arith.ori %808, %847 : i1
    %849 = arith.andi %845, %848 : i1
    %850 = arith.andi %807, %797 : i1
    %851 = arith.cmpi ne, %789, %c1_i32 : i32 loc(#loc36)
    %852 = arith.andi %807, %851 : i1
    %853 = arith.xori %852, %true : i1
    %854 = arith.ori %853, %847 : i1
    %855 = arith.andi %854, %841 : i1
    %856 = arith.ori %853, %855 : i1
    %857 = arith.andi %849, %856 : i1
    %858 = arith.andi %852, %798 : i1
    %859 = arith.cmpi ne, %789, %c2_i32 : i32 loc(#loc36)
    %860 = arith.andi %852, %859 : i1
    %861 = arith.xori %860, %true : i1
    %862 = arith.ori %861, %855 : i1
    %863 = arith.xori %858, %true : i1 loc(#loc36)
    %864 = arith.ori %863, %855 : i1
    acir.path %858 {branch = 17 : i64, kind = "branch", txn = 3 : i64} loc(#loc36)
    acir.path %858 {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc36)
    %865 = arith.ori %863, %864 : i1 loc(#loc36)
    %value_32, %ready_33 = acir.read(%858) {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc37)
    %866 = arith.andi %865, %ready_33 : i1 loc(#loc37)
    %867 = arith.ori %863, %866 : i1 loc(#loc37)
    %868 = arith.andi %857, %867 : i1 loc(#loc37)
    %869 = acir.extract %value_32 {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc37)
    %870 = arith.andi %868, %867 : i1
    %871 = arith.andi %858, %869 : i1
    %872 = arith.xori %869, %true : i1
    %873 = arith.andi %858, %872 : i1
    %874 = arith.xori %873, %true : i1
    %875 = arith.ori %874, %866 : i1
    %876 = arith.xori %871, %true : i1 loc(#loc38)
    %877 = arith.ori %876, %866 : i1
    acir.path %871 {branch = 18 : i64, kind = "branch", txn = 3 : i64} loc(#loc38)
    %value_34, %ready_35 = acir.read(%871) {branch = 18 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc38)
    %878 = arith.andi %877, %ready_35 : i1 loc(#loc38)
    %879 = arith.ori %876, %878 : i1 loc(#loc38)
    %880 = arith.andi %870, %879 : i1 loc(#loc38)
    %881 = acir.extract %value_34 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc38)
    %882 = arith.andi %873, %875 : i1
    %883 = arith.andi %871, %878 : i1
    %884 = arith.ori %882, %883 : i1
    %885 = arith.select %873, %false, %881 : i1
    %886 = arith.ori %863, %865 : i1
    %887 = arith.andi %886, %884 : i1 loc(#loc37)
    %888 = arith.andi %858, %887 : i1 loc(#loc37)
    %889 = arith.andi %888, %885 : i1 loc(#loc37)
    %890 = arith.andi %864, %889 : i1 loc(#loc37)
    %891 = arith.ori %863, %890 : i1 loc(#loc37)
    %892 = arith.andi %880, %891 : i1 loc(#loc37)
    %value_36, %ready_37 = acir.read(%858) {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc39)
    %893 = arith.andi %891, %ready_37 : i1 loc(#loc39)
    %894 = arith.ori %863, %893 : i1 loc(#loc39)
    %895 = arith.andi %892, %894 : i1 loc(#loc39)
    %896 = acir.extract %value_36 {acir.id = "v18", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc39)
    %897 = acir.aggregate(%false, %false, %c0_i32) {acir.id = "v22", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc40)
    %898 = arith.ori %863, %891 : i1 loc(#loc41)
    %899 = arith.andi %895, %898 : i1 loc(#loc41)
    %900 = arith.andi %899, %898 : i1
    %901 = arith.andi %858, %893 : i1
    %902 = arith.andi %901, %889 : i1
    %903 = arith.andi %860, %862 : i1
    %904 = arith.andi %858, %864 : i1
    %905 = arith.ori %903, %904 : i1
    %906 = arith.andi %864, %902 : i1
    %907 = arith.andi %858, %906 : i1
    %908 = arith.ori %903, %907 : i1
    %909 = arith.select %860, %842, %896 : i32
    %910 = arith.ori %853, %854 : i1
    %911 = arith.xori %850, %true : i1 loc(#loc36)
    %912 = arith.ori %911, %847 : i1
    acir.path %850 {branch = 19 : i64, kind = "branch", txn = 3 : i64} loc(#loc36)
    acir.path %850 {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc36)
    %913 = arith.ori %911, %912 : i1 loc(#loc36)
    %value_38, %ready_39 = acir.read(%850) {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc37)
    %914 = arith.andi %913, %ready_39 : i1 loc(#loc37)
    %915 = arith.ori %911, %914 : i1 loc(#loc37)
    %916 = arith.andi %900, %915 : i1 loc(#loc37)
    %917 = acir.extract %value_38 {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc37)
    %918 = arith.andi %916, %915 : i1
    %919 = arith.andi %850, %917 : i1
    %920 = arith.xori %917, %true : i1
    %921 = arith.andi %850, %920 : i1
    %922 = arith.xori %921, %true : i1
    %923 = arith.ori %922, %914 : i1
    %924 = arith.xori %919, %true : i1 loc(#loc38)
    %925 = arith.ori %924, %914 : i1
    acir.path %919 {branch = 20 : i64, kind = "branch", txn = 3 : i64} loc(#loc38)
    %value_40, %ready_41 = acir.read(%919) {branch = 20 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc38)
    %926 = arith.andi %925, %ready_41 : i1 loc(#loc38)
    %927 = arith.ori %924, %926 : i1 loc(#loc38)
    %928 = arith.andi %918, %927 : i1 loc(#loc38)
    %929 = acir.extract %value_40 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc38)
    %930 = arith.andi %921, %923 : i1
    %931 = arith.andi %919, %926 : i1
    %932 = arith.ori %930, %931 : i1
    %933 = arith.select %921, %false, %929 : i1
    %934 = arith.ori %911, %913 : i1
    %935 = arith.andi %934, %932 : i1 loc(#loc37)
    %936 = arith.andi %850, %935 : i1 loc(#loc37)
    %937 = arith.andi %936, %933 : i1 loc(#loc37)
    %938 = arith.andi %912, %937 : i1 loc(#loc37)
    %939 = arith.ori %911, %938 : i1 loc(#loc37)
    %940 = arith.andi %928, %939 : i1 loc(#loc37)
    %value_42, %ready_43 = acir.read(%850) {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "read", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc39)
    %941 = arith.andi %939, %ready_43 : i1 loc(#loc39)
    %942 = arith.ori %911, %941 : i1 loc(#loc39)
    %943 = arith.andi %940, %942 : i1 loc(#loc39)
    %944 = acir.extract %value_42 {acir.id = "v18", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc39)
    %945 = arith.ori %911, %939 : i1 loc(#loc41)
    %946 = arith.andi %943, %945 : i1 loc(#loc41)
    %947 = arith.andi %946, %945 : i1
    %948 = arith.andi %850, %941 : i1
    %949 = arith.andi %948, %937 : i1
    %950 = arith.andi %910, %905 : i1
    %951 = arith.andi %852, %950 : i1
    %952 = arith.andi %850, %912 : i1
    %953 = arith.ori %951, %952 : i1
    %954 = arith.select %852, %860, %false : i1
    %955 = arith.andi %910, %908 : i1
    %956 = arith.andi %852, %955 : i1
    %957 = arith.andi %912, %949 : i1
    %958 = arith.andi %850, %957 : i1
    %959 = arith.ori %956, %958 : i1
    %960 = arith.select %852, %909, %944 : i32
    %961 = arith.ori %808, %846 : i1
    %962 = arith.ori %810, %843 : i1
    acir.path %805 {branch = 21 : i64, kind = "branch", txn = 3 : i64} loc(#loc36)
    acir.path %805 {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc36)
    %963 = arith.ori %810, %962 : i1 loc(#loc36)
    %value_44, %ready_45 = acir.read(%805) {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc37)
    %964 = arith.andi %963, %ready_45 : i1 loc(#loc37)
    %965 = arith.ori %810, %964 : i1 loc(#loc37)
    %966 = arith.andi %947, %965 : i1 loc(#loc37)
    %967 = acir.extract %value_44 {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc37)
    %968 = arith.andi %966, %965 : i1
    %969 = arith.andi %805, %967 : i1
    %970 = arith.xori %967, %true : i1
    %971 = arith.andi %805, %970 : i1
    %972 = arith.xori %971, %true : i1
    %973 = arith.ori %972, %964 : i1
    %974 = arith.xori %969, %true : i1 loc(#loc38)
    %975 = arith.ori %974, %964 : i1
    acir.path %969 {branch = 22 : i64, kind = "branch", txn = 3 : i64} loc(#loc38)
    %value_46, %ready_47 = acir.read(%969) {branch = 22 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc38)
    %976 = arith.andi %975, %ready_47 : i1 loc(#loc38)
    %977 = arith.ori %974, %976 : i1 loc(#loc38)
    %978 = arith.andi %968, %977 : i1 loc(#loc38)
    %979 = acir.extract %value_46 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc38)
    %980 = arith.andi %971, %973 : i1
    %981 = arith.andi %969, %976 : i1
    %982 = arith.ori %980, %981 : i1
    %983 = arith.select %971, %false, %979 : i1
    %984 = arith.ori %810, %963 : i1
    %985 = arith.andi %984, %982 : i1 loc(#loc37)
    %986 = arith.andi %805, %985 : i1 loc(#loc37)
    %987 = arith.andi %986, %983 : i1 loc(#loc37)
    %988 = arith.andi %962, %987 : i1 loc(#loc37)
    %989 = arith.ori %810, %988 : i1 loc(#loc37)
    %990 = arith.andi %978, %989 : i1 loc(#loc37)
    %value_48, %ready_49 = acir.read(%805) {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "read", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc39)
    %991 = arith.andi %989, %ready_49 : i1 loc(#loc39)
    %992 = arith.ori %810, %991 : i1 loc(#loc39)
    %993 = arith.andi %990, %992 : i1 loc(#loc39)
    %994 = acir.extract %value_48 {acir.id = "v18", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc39)
    %995 = arith.ori %810, %989 : i1 loc(#loc41)
    %996 = arith.andi %993, %995 : i1 loc(#loc41)
    %997 = arith.andi %996, %995 : i1
    %998 = arith.andi %805, %991 : i1
    %999 = arith.andi %998, %987 : i1
    %1000 = arith.andi %961, %953 : i1
    %1001 = arith.andi %807, %1000 : i1
    %1002 = arith.andi %805, %962 : i1
    %1003 = arith.ori %1001, %1002 : i1
    %1004 = arith.select %807, %954, %false : i1
    %1005 = arith.andi %961, %959 : i1
    %1006 = arith.andi %807, %1005 : i1
    %1007 = arith.andi %962, %999 : i1
    %1008 = arith.andi %805, %1007 : i1
    %1009 = arith.ori %1006, %1008 : i1
    %1010 = arith.select %807, %960, %994 : i32
    %1011 = arith.ori %794, %838 : i1
    %1012 = acir.unary %1004 "not" {acir.source_type = "bool"} : i1 -> i1
    %1013 = arith.andi %1011, %1003 : i1
    %1014 = arith.ori %794, %1013 : i1
    %1015 = arith.andi %997, %1014 : i1
    %1016 = arith.andi %1011, %1009 : i1
    %1017 = arith.andi %793, %1016 : i1
    %1018 = arith.andi %1017, %836 : i1
    acir.path %793 {branch = 16 : i64, frames = ["top:issue", "top.age:release"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc42)
    %value_50, %ready_51 = acir.read(%793) {branch = 16 : i64, frames = ["top:issue", "top.age:release"], method = "read", port = 0 : i64, resource = "top.age.used", txn = 3 : i64} : (i1) -> (!acir.array<i1, [3]>, i1) loc(#loc43)
    %1019 = arith.andi %799, %ready_51 : i1 loc(#loc43)
    %1020 = arith.ori %794, %1019 : i1 loc(#loc43)
    %1021 = arith.andi %1015, %1020 : i1 loc(#loc43)
    %1022 = arith.andi %ready_51, %801 : i1 loc(#loc44)
    %1023 = arith.extui %789 : i32 to i64 loc(#loc44)
    %1024 = arith.cmpi ult, %1023, %c3_i64 : i64 loc(#loc44)
    %1025 = arith.andi %1022, %1024 : i1 loc(#loc44)
    %1026 = arith.select %1024, %1023, %c0_i64 : i64 loc(#loc44)
    %1027 = acir.update %value_50, %false[%1026] {acir.id = "v8", acir.source_type = "array<bool;3>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i64) -> !acir.array<i1, [3]> loc(#loc44)
    %1028 = arith.andi %799, %1025 : i1 loc(#loc45)
    %1029 = arith.ori %794, %1028 : i1 loc(#loc45)
    %1030 = arith.andi %1021, %1029 : i1 loc(#loc45)
    %1031 = arith.andi %795, %1018 : i1 loc(#loc46)
    %1032 = arith.ori %794, %1031 : i1 loc(#loc46)
    %1033 = arith.andi %1030, %1032 : i1 loc(#loc46)
    %value_52, %ready_53 = acir.read(%793) {branch = 16 : i64, frames = ["top:issue"], method = "full", port = 0 : i64, resource = "top.output", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc46)
    %1034 = arith.andi %795, %ready_53 : i1 loc(#loc46)
    %1035 = arith.ori %794, %1034 : i1 loc(#loc46)
    %1036 = arith.andi %1033, %1035 : i1 loc(#loc46)
    %1037 = arith.xori %value_52, %true : i1 loc(#loc46)
    %1038 = arith.andi %795, %1037 : i1 loc(#loc46)
    %1039 = arith.ori %794, %1038 : i1 loc(#loc46)
    %1040 = arith.andi %1036, %1039 : i1 loc(#loc46)
    %value_54, %ready_55 = acir.read(%793) {branch = 16 : i64, frames = ["top:issue"], method = "read", port = 0 : i64, resource = "top.issued", txn = 3 : i64} : (i1) -> (i32, i1) loc(#loc47)
    %1041 = arith.andi %795, %ready_55 : i1 loc(#loc47)
    %1042 = arith.ori %794, %1041 : i1 loc(#loc47)
    %1043 = arith.andi %1040, %1042 : i1 loc(#loc47)
    %1044 = arith.addi %value_54, %c1_i32 {acir.id = "v23", acir.source_type = "u32"} : i32 loc(#loc47)
    %1045 = arith.andi %1043, %1042 : i1 loc(#loc48)
    %1046 = acir.fire "top.issue#3" %1045 {successors = [], txn = 3 : i64}
    %1047 = arith.andi %1046, %271 : i1
    acir.effect %1047(%261) {branch = 15 : i64, frames = ["top:issue", "top.age:choose"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
    %1048 = arith.andi %1046, %858 : i1
    acir.effect %1048(%897) {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 3 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc41)
    acir.effect %1048(%261) {branch = 17 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.2:take"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
    %1049 = arith.andi %1046, %850 : i1
    acir.effect %1049(%897) {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "write", port = 0 : i64, resource = "top.slots.1.state", txn = 3 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc41)
    acir.effect %1049(%261) {branch = 19 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.1:take"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
    %1050 = arith.andi %1046, %805 : i1
    acir.effect %1050(%897) {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "write", port = 0 : i64, resource = "top.slots.0.state", txn = 3 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc41)
    acir.effect %1050(%261) {branch = 21 : i64, frames = ["top:issue", "top:$dispatch10", "top.slots.0:take"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
    %1051 = arith.andi %1046, %793 : i1
    acir.effect %1051(%1012) {branch = 16 : i64, frames = ["top:issue", "top:$dispatch10"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
    acir.effect %1051(%1027) {branch = 16 : i64, frames = ["top:issue", "top.age:release"], method = "write", port = 0 : i64, resource = "top.age.used", txn = 3 : i64} : (i1, !acir.array<i1, [3]>) -> () loc(#loc45)
    acir.effect %1051(%1010) {branch = 16 : i64, frames = ["top:issue"], method = "push", port = 0 : i64, resource = "top.output", txn = 3 : i64} : (i1, i32) -> () loc(#loc46)
    acir.effect %1051(%1044) {branch = 16 : i64, frames = ["top:issue"], method = "write", port = 0 : i64, resource = "top.issued", txn = 3 : i64} : (i1, i32) -> () loc(#loc48)
    %value_56, %ready_57 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc49)
    %1052 = arith.cmpi eq, %value_56, %c5_i32 {acir.id = "v101", acir.source_type = "bool"} : i32 loc(#loc49)
    %1053 = arith.cmpi ne, %value_56, %c5_i32 : i32 loc(#loc49)
    %1054 = arith.ori %1053, %ready_57 : i1
    %1055 = arith.andi %1052, %1054 : i1 loc(#loc50)
    acir.path %1052 {branch = 23 : i64, kind = "branch", txn = 4 : i64} loc(#loc50)
    acir.path %1052 {branch = 23 : i64, frames = ["top.slots.2:wake"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc50)
    %1056 = arith.ori %1053, %1054 : i1 loc(#loc50)
    %value_58, %ready_59 = acir.read(%1052) {branch = 23 : i64, frames = ["top.slots.2:wake"], method = "read", port = 0 : i64, resource = "top.slots.2.state", txn = 4 : i64} : (i1) -> (!acir.struct<"src_array_arbiter_1242b928.Entry">, i1) loc(#loc51)
    %1057 = arith.andi %1056, %ready_59 : i1 loc(#loc51)
    %1058 = arith.ori %1053, %1057 : i1 loc(#loc51)
    %1059 = arith.andi %1055, %1058 : i1 loc(#loc51)
    %1060 = acir.extract %value_58 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc52)
    %1061 = arith.andi %1059, %1058 : i1
    %1062 = arith.andi %1052, %1060 : i1
    %1063 = arith.xori %1062, %true : i1 loc(#loc53)
    %1064 = arith.ori %1063, %1057 : i1
    %1065 = acir.extract %value_58 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc54)
    %1066 = acir.extract %value_58 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc54)
    %1067 = acir.aggregate(%1065, %true, %1066) {acir.id = "v12", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc54)
    %1068 = arith.andi %1064, %ready_59 : i1 loc(#loc53)
    %1069 = arith.ori %1063, %1068 : i1 loc(#loc53)
    %1070 = arith.andi %1061, %1069 : i1 loc(#loc53)
    acir.path %1062 {branch = 24 : i64, kind = "branch", txn = 4 : i64} loc(#loc53)
    %1071 = acir.fire "top.slots.2.wake#4" %1070 {successors = [], txn = 4 : i64}
    %1072 = arith.andi %1071, %1062 : i1
    acir.effect %1072(%1067) {branch = 24 : i64, frames = ["top.slots.2:wake"], method = "write", port = 0 : i64, resource = "top.slots.2.state", txn = 4 : i64} : (i1, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> () loc(#loc53)
    %value_60, %ready_61 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc55)
    %1073 = arith.cmpi uge, %value_60, %c4_i32 {acir.id = "v110", acir.source_type = "bool"} : i32 loc(#loc55)
    %1074 = arith.cmpi ult, %value_60, %c4_i32 : i32 loc(#loc55)
    %1075 = arith.cmpi uge, %value_60, %c4_i32 : i32 loc(#loc55)
    %1076 = arith.ori %1075, %ready_61 : i1
    %1077 = arith.ori %1074, %ready_61 : i1
    acir.path %1073 {branch = 25 : i64, kind = "branch", txn = -1 : i64} loc(#loc56)
    %value_62, %ready_63 = acir.read(%1073) {branch = 25 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cycle", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc56)
    %1078 = arith.remui %value_62, %c2_i32 : i32 loc(#loc56)
    %1079 = arith.cmpi eq, %1078, %c0_i32 {acir.id = "v116", acir.source_type = "bool"} : i32 loc(#loc56)
    %1080 = arith.andi %1074, %1076 : i1
    %1081 = arith.andi %1077, %ready_63 : i1
    %1082 = arith.andi %1073, %1081 : i1
    %1083 = arith.ori %1080, %1082 : i1
    %1084 = arith.select %1074, %false, %1079 : i1
    %1085 = arith.xori %1084, %true : i1 loc(#loc57)
    %1086 = arith.ori %1085, %1083 : i1
    %1087 = arith.andi %1084, %1086 : i1 loc(#loc57)
    acir.path %1084 {branch = 26 : i64, kind = "branch", txn = 5 : i64} loc(#loc57)
    acir.path %1084 {branch = 26 : i64, frames = ["top:drain"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc57)
    %1088 = arith.ori %1085, %1086 : i1 loc(#loc57)
    %value_64, %ready_65 = acir.read(%1084) {branch = 26 : i64, frames = ["top:drain"], method = "peek", port = 0 : i64, resource = "top.output", txn = 5 : i64} : (i1) -> (i32, i1) loc(#loc58)
    %1089 = arith.andi %1088, %ready_65 : i1 loc(#loc58)
    %1090 = arith.ori %1085, %1089 : i1 loc(#loc58)
    %1091 = arith.andi %1087, %1090 : i1 loc(#loc58)
    %1092 = arith.andi %1091, %1090 : i1 loc(#loc58)
    %value_66, %ready_67 = acir.read(%1084) {branch = 26 : i64, frames = ["top:drain"], method = "empty", port = 0 : i64, resource = "top.output", txn = 5 : i64} : (i1) -> (i1, i1) loc(#loc58)
    %1093 = arith.andi %1088, %ready_67 : i1 loc(#loc58)
    %1094 = arith.ori %1085, %1093 : i1 loc(#loc58)
    %1095 = arith.andi %1092, %1094 : i1 loc(#loc58)
    %1096 = arith.xori %value_66, %true : i1 loc(#loc58)
    %1097 = arith.andi %1088, %1096 : i1 loc(#loc58)
    %1098 = arith.ori %1085, %1097 : i1 loc(#loc58)
    %1099 = arith.andi %1095, %1098 : i1 loc(#loc58)
    %1100 = arith.andi %1099, %1090 : i1 loc(#loc59)
    %value_68, %ready_69 = acir.read(%1084) {branch = 26 : i64, frames = ["top:drain"], method = "read", port = 0 : i64, resource = "top.retired", txn = 5 : i64} : (i1) -> (i32, i1) loc(#loc60)
    %1101 = arith.andi %1088, %ready_69 : i1 loc(#loc60)
    %1102 = arith.ori %1085, %1101 : i1 loc(#loc60)
    %1103 = arith.andi %1100, %1102 : i1 loc(#loc60)
    %1104 = arith.addi %value_68, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc60)
    %1105 = arith.andi %1103, %1102 : i1 loc(#loc61)
    %1106 = acir.fire "top.drain#5" %1105 {successors = [3], txn = 5 : i64}
    %1107 = arith.andi %1106, %1084 : i1
    acir.effect %1107() {branch = 26 : i64, frames = ["top:drain"], method = "pop", port = 0 : i64, resource = "top.output", txn = 5 : i64} : (i1) -> () loc(#loc58)
    acir.effect %1107(%value_64) {branch = 26 : i64, frames = ["top:drain"], method = "write", port = 0 : i64, resource = "top.last", txn = 5 : i64} : (i1, i32) -> () loc(#loc59)
    acir.effect %1107(%1104) {branch = 26 : i64, frames = ["top:drain"], method = "write", port = 0 : i64, resource = "top.retired", txn = 5 : i64} : (i1, i32) -> () loc(#loc61)
    acir.path %true {branch = 27 : i64, kind = "branch", txn = 6 : i64} loc(#loc62)
    acir.path %true {branch = 27 : i64, frames = ["top:clock"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc62)
    %value_70, %ready_71 = acir.read(%true) {branch = 27 : i64, frames = ["top:clock"], method = "read", port = 0 : i64, resource = "top.cycle", txn = 6 : i64} : (i1) -> (i32, i1) loc(#loc63)
    %1108 = arith.addi %value_70, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc63)
    %1109 = acir.fire "top.clock#6" %ready_71 {successors = [], txn = 6 : i64}
    acir.effect %1109(%1108) {branch = 27 : i64, frames = ["top:clock"], method = "write", port = 0 : i64, resource = "top.cycle", txn = 6 : i64} : (i1, i32) -> () loc(#loc64)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":115:12)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":117:14)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":119:14)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:13)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":93:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:17)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:24)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:20)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":94:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":49:17)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":50:18)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:35)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:13)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":53:13)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":54:9)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":55:9)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":56:9)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:13)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:13)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":121:29)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":26:16)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":121:28)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":122:12)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:27)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:55)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:71)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:19)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:13)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":98:27)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":66:16)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":67:18)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:20)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:32)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":78:9)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":100:21)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:20)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:36)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":37:17)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:17)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:9)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":101:13)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":60:17)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":61:9)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":62:9)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":102:13)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:22)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:13)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":124:12)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":125:13)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":30:17)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":31:12)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":33:13)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":32:13)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:12)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:27)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":127:13)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:16)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:9)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:19)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:9)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":128:9)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:17)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:9)

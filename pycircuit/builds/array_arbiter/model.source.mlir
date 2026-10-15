#loc10 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12:value")
#loc11 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12:ready")
#loc12 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable0")
#loc13 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable1")
#loc14 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable2")
#loc16 = loc("acir.binding:top_body.0.call.0.8:position")
#loc17 = loc("acir.binding:top_body.0.call.0.8:value")
#loc18 = loc("acir.binding:top_body.0.call.0.8:ready")
#loc20 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable0")
#loc21 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable1")
#loc22 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable2")
#loc24 = loc("acir.binding:top_body.0.call.9.94:candidates")
#loc80 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.0.16:value")
#loc81 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.0.16:ready")
#loc93 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.1.18:value")
#loc94 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.1.18:ready")
#loc96 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.2.20:value")
#loc97 = loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.2.20:ready")
#loc103 = loc("acir.binding:top_body.0.call.0.8.0.call.4.13:position")
#loc109 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:candidates")
#loc119 = loc("acir.binding:top_body.0.call.9.94.0.call.14.19:position")
module attributes {acir.model = {config = [], constants = {src_array_arbiter_1242b928.N = 3 : i64}, format_version = 9 : i64, top = "ArrayArbiter", type_order = ["src_array_arbiter_1242b928.Entry"], types = {src_array_arbiter_1242b928.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}} {
  func.func @ac_init_top.slots.0.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.slots.0.state", params = [], result = "src_array_arbiter_1242b928.Entry"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    %0 = acir.aggregate(%false, %false, %c0_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc1)
    return %0 : !acir.struct<"src_array_arbiter_1242b928.Entry">
  }
  func.func @ac_init_top.slots.1.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.slots.1.state", params = [], result = "src_array_arbiter_1242b928.Entry"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    %0 = acir.aggregate(%false, %false, %c0_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc1)
    return %0 : !acir.struct<"src_array_arbiter_1242b928.Entry">
  }
  func.func @ac_init_top.slots.2.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.slots.2.state", params = [], result = "src_array_arbiter_1242b928.Entry"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    %0 = acir.aggregate(%false, %false, %c0_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc1)
    return %0 : !acir.struct<"src_array_arbiter_1242b928.Entry">
  }
  func.func @ac_init_top.age.used() -> !acir.array<i1, [3]> attributes {acir.function = {kind = "helper", name = "ac_init_top.age.used", params = [], result = "array<bool;3>"}} {
    %false = arith.constant false
    %0 = acir.aggregate(%false, %false, %false) : (i1, i1, i1) -> !acir.array<i1, [3]> loc(#loc2)
    return %0 : !acir.array<i1, [3]>
  }
  func.func @ac_init_top.age.older() -> !acir.array<i1, [3, 3]> attributes {acir.function = {kind = "helper", name = "ac_init_top.age.older", params = [], result = "array<bool;3x3>"}} {
    %false = arith.constant false
    %0 = acir.aggregate(%false, %false, %false) : (i1, i1, i1) -> !acir.array<i1, [3]> loc(#loc3)
    %1 = acir.aggregate(%0, %0, %0) : (!acir.array<i1, [3]>, !acir.array<i1, [3]>, !acir.array<i1, [3]>) -> !acir.array<i1, [3, 3]> loc(#loc4)
    return %1 : !acir.array<i1, [3, 3]>
  }
  func.func @ac_init_top.output_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.output_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.output() -> tuple<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.output", params = [], result = "tuple<u32>"}} {
    %c99_i32 = arith.constant 99 : i32 loc(#loc5)
    %0 = acir.aggregate(%c99_i32) : (i32) -> tuple<i32>
    return %0 : tuple<i32>
  }
  func.func @ac_init_top.cycle() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cycle", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc6)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.issued() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.issued", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc7)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.last() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.last", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc8)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc9)
    return %c0_i32 : i32
  }
  func.func private @ac_builtin_mod_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "mod", kind = "intrinsic", name = "ac_builtin_mod_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  acir.module @top attributes {acir.instance = {definition = "src_array_arbiter_1242b928.ArrayArbiter", name = "top", parent, rules = ["allocate", "clock", "drain", "issue"]}} {
    acir.contract {relations = [], rules = ["allocate", "clock", "drain", "issue"]}
    acir.queue @top.output : i32 {capacity_fn = "ac_init_top.output_cap", initial_fn = "ac_init_top.output", name = "top.output", type = "u32"}
    acir.cell @top.cycle : i32 {initial_fn = "ac_init_top.cycle", name = "top.cycle", type = "u32"}
    acir.cell @top.issued : i32 {initial_fn = "ac_init_top.issued", name = "top.issued", type = "u32"}
    acir.cell @top.last : i32 {initial_fn = "ac_init_top.last", name = "top.last", type = "u32"}
    acir.cell @top.retired : i32 {initial_fn = "ac_init_top.retired", name = "top.retired", type = "u32"}
    "acir.instance.array"() <{name = "top.slots", shape = [3], targets = [@top.slots.0, @top.slots.1, @top.slots.2]}> : () -> ()
    func.func private @ac_init_top.slots.0.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.1.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.2.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.age.used() -> !acir.array<i1, [3]>
    func.func private @ac_init_top.age.older() -> !acir.array<i1, [3, 3]>
    func.func private @ac_init_top.output_cap() -> i32
    func.func private @ac_init_top.output() -> tuple<i32>
    func.func private @ac_init_top.cycle() -> i32
    func.func private @ac_init_top.issued() -> i32
    func.func private @ac_init_top.last() -> i32
    func.func private @ac_init_top.retired() -> i32
    func.func private @ac_builtin_mod_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "mod", kind = "intrinsic", name = "ac_builtin_mod_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top_body.0.call.0.8.0.call.0.12 : (i32, i1, i1, i1, i1) -> () {acir.function = {contract = "Module:top", definition = "$dispatch0", guard_info = {name = "", source = "variable"}, instance = "top", interface = "Module:top:$dispatch0", kind = "rule", name = "top_body.0.call.0.8.0.call.0.12", output_type = "void", owner = "top", params = [{id = "v1", name = "value", targets = [], type = "u32"}, {id = "v2", name = "ready", targets = [], type = "bool"}, {id = "v3", name = "$enable0", targets = [], type = "bool"}, {id = "v4", name = "$enable1", targets = [], type = "bool"}, {id = "v5", name = "$enable2", targets = [], type = "bool"}], public = true, result = "void", rule = "$dispatch0"}} guard {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:ready"), %arg2: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable0"), %arg3: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable1"), %arg4: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable2")):
      %true = arith.constant true
      %0 = arith.select %arg2, %true, %arg3 : i1
      %1 = arith.select %0, %true, %arg4 : i1
      acir.guard.yield %1 {acir.id = "v15", acir.source_type = "void"} loc(#loc15)
    } body {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:ready"), %arg2: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable0"), %arg3: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable1"), %arg4: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12:$enable2")):
      scf.if %arg2 {
        acir.rule.call @top.slots.0::@top_body.0.call.0.8.0.call.0.12.0.call.0.16(%arg0, %arg1) {acir.id = "v17", acir.source_type = "void", contract = "Module:top.slots.0", instance = "top.slots.0", interface = "Module:top.slots.0:allocate", role = "rule", rule = "allocate"} : (i32, i1) -> () loc(#loc15)
      } else {
        scf.if %arg3 {
          acir.rule.call @top.slots.1::@top_body.0.call.0.8.0.call.0.12.0.call.1.18(%arg0, %arg1) {acir.id = "v19", acir.source_type = "void", contract = "Module:top.slots.1", instance = "top.slots.1", interface = "Module:top.slots.1:allocate", role = "rule", rule = "allocate"} : (i32, i1) -> () loc(#loc15)
        } else {
          scf.if %arg4 {
            acir.rule.call @top.slots.2::@top_body.0.call.0.8.0.call.0.12.0.call.2.20(%arg0, %arg1) {acir.id = "v21", acir.source_type = "void", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:allocate", role = "rule", rule = "allocate"} : (i32, i1) -> () loc(#loc15)
          }
        }
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.0.8 : (i32, i32, i1) -> () {acir.function = {contract = "Module:top", definition = "allocate", instance = "top", interface = "Module:top:allocate", kind = "rule", name = "top_body.0.call.0.8", output_type = "void", owner = "top", params = [{id = "v1", name = "position", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}, {id = "v3", name = "ready", targets = [], type = "bool"}], public = true, result = "void", rule = "allocate"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8:position"), %arg1: i32 loc("acir.binding:top_body.0.call.0.8:value"), %arg2: i1 loc("acir.binding:top_body.0.call.0.8:ready")):
      %c2_i32 = arith.constant 2 : i32
      %c1_i32 = arith.constant 1 : i32
      %c0_i32 = arith.constant 0 : i32
      %0 = arith.cmpi eq, %arg0, %c0_i32 : i32 loc(#loc15)
      %1 = arith.cmpi eq, %arg0, %c1_i32 : i32 loc(#loc15)
      %2 = arith.cmpi eq, %arg0, %c2_i32 : i32 loc(#loc15)
      acir.rule.call @top::@top_body.0.call.0.8.0.call.0.12(%arg1, %arg2, %0, %1, %2) {acir.id = "v13", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:$dispatch0", role = "rule", rule = "$dispatch0"} : (i32, i1, i1, i1, i1) -> () loc(#loc15)
      acir.rule.call @top.age::@top_body.0.call.0.8.0.call.4.13(%arg0) {acir.id = "v14", acir.source_type = "void", contract = "Module:top.age", instance = "top.age", interface = "Module:top.age:allocate", role = "rule", rule = "allocate"} : (i32) -> () loc(#loc19)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.94.0.call.10.17 : (i1, i1, i1) -> i32 {acir.function = {contract = "Module:top", definition = "$dispatch10", guard_info = {name = "", source = "variable"}, instance = "top", interface = "Module:top:$dispatch10", kind = "rule", name = "top_body.0.call.9.94.0.call.10.17", output_type = "u32", owner = "top", params = [{id = "v1", name = "$enable0", targets = [], type = "bool"}, {id = "v2", name = "$enable1", targets = [], type = "bool"}, {id = "v3", name = "$enable2", targets = [], type = "bool"}], public = true, result = "u32", rule = "$dispatch10"}} guard {
    ^bb0(%arg0: i1 loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable0"), %arg1: i1 loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable1"), %arg2: i1 loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable2")):
      %true = arith.constant true
      %0 = arith.select %arg0, %true, %arg1 : i1
      %1 = arith.select %0, %true, %arg2 : i1
      acir.guard.yield %1 {acir.id = "v13", acir.source_type = "void"} loc(#loc23)
    } body {
    ^bb0(%arg0: i1 loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable0"), %arg1: i1 loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable1"), %arg2: i1 loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable2")):
      %true = arith.constant true
      %false = arith.constant false
      %0 = acir.aggregate() : () -> i32
      %1:2 = scf.if %arg0 -> (i1, i32) {
        %3 = acir.rule.call @top.slots.0::@top_body.0.call.9.94.0.call.10.17.0.call.10.16() {acir.id = "v17", acir.source_type = "u32", contract = "Module:top.slots.0", instance = "top.slots.0", interface = "Module:top.slots.0:take", role = "rule", rule = "take"} : () -> i32 loc(#loc23)
        scf.yield %false, %3 : i1, i32
      } else {
        %3:2 = scf.if %arg1 -> (i1, i32) {
          %4 = acir.rule.call @top.slots.1::@top_body.0.call.9.94.0.call.10.17.0.call.11.19() {acir.id = "v20", acir.source_type = "u32", contract = "Module:top.slots.1", instance = "top.slots.1", interface = "Module:top.slots.1:take", role = "rule", rule = "take"} : () -> i32 loc(#loc23)
          scf.yield %false, %4 : i1, i32
        } else {
          %4 = arith.xori %arg2, %true : i1
          %5 = scf.if %arg2 -> (i32) {
            %6 = acir.rule.call @top.slots.2::@top_body.0.call.9.94.0.call.10.17.0.call.12.22() {acir.id = "v23", acir.source_type = "u32", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:take", role = "rule", rule = "take"} : () -> i32 loc(#loc23)
            scf.yield %6 : i32
          } else {
            scf.yield %0 : i32
          }
          scf.yield %4, %5 : i1, i32
        }
        scf.yield %3#0, %3#1 : i1, i32
      }
      %2 = arith.xori %1#0, %true : i1
      acir.check %2 {acir.source_type = "void"}
      acir.return %1#1 : i32 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.94 : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> () {acir.function = {contract = "Module:top", definition = "issue", instance = "top", interface = "Module:top:issue", kind = "rule", name = "top_body.0.call.9.94", output_type = "void", owner = "top", params = [{id = "v1", name = "candidates", targets = [], type = "selected<array<src_array_arbiter_1242b928.Entry;3>>"}], public = true, result = "void", rule = "issue"}} body {
    ^bb0(%arg0: !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>> loc("acir.binding:top_body.0.call.9.94:candidates")):
      %c2_i32 = arith.constant 2 : i32
      %c1_i32 = arith.constant 1 : i32
      %c0_i32 = arith.constant 0 : i32
      %0 = acir.rule.call @top.age::@top_body.0.call.9.94.0.call.9.4(%arg0) {acir.id = "v5", acir.source_type = "tuple<bool,u32>", contract = "Module:top.age", instance = "top.age", interface = "Module:top.age:choose", role = "rule", rule = "choose"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> tuple<i1, i32> loc(#loc25)
      %1 = acir.extract %0[%c0_i32] {indexed = true, output_call = "v5", slot = 0 : i64} : (tuple<i1, i32>, i32) -> i1 loc(#loc26)
      %2 = acir.extract %0[%c1_i32] {indexed = true, output_call = "v5", slot = 1 : i64} : (tuple<i1, i32>, i32) -> i32 loc(#loc27)
      %3 = acir.ref @top::@top.output : !acir.queue<i32>
      %4 = acir.ref @top::@top.issued : !acir.cell<i32>
      scf.if %1 {
        %5 = arith.cmpi eq, %2, %c0_i32 : i32 loc(#loc23)
        %6 = arith.cmpi eq, %2, %c1_i32 : i32 loc(#loc23)
        %7 = arith.cmpi eq, %2, %c2_i32 : i32 loc(#loc23)
        %8 = acir.rule.call @top::@top_body.0.call.9.94.0.call.10.17(%5, %6, %7) {acir.id = "v18", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:$dispatch10", role = "rule", rule = "$dispatch10"} : (i1, i1, i1) -> i32 loc(#loc28)
        acir.rule.call @top.age::@top_body.0.call.9.94.0.call.14.19(%2) {acir.id = "v20", acir.source_type = "void", contract = "Module:top.age", instance = "top.age", interface = "Module:top.age:release", role = "rule", rule = "release"} : (i32) -> () loc(#loc29)
        acir.queue.push %3, %8 {acir.id = "v22", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc30)
        %9 = acir.cell.read %4 {acir.id = "v24", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc31)
        %10 = arith.addi %9, %c1_i32 : i32 loc(#loc31)
        acir.cell.write %4, %10 {acir.id = "v28", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc32)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.17.121 : () -> () {acir.function = {contract = "Module:top", definition = "drain", instance = "top", interface = "Module:top:drain", kind = "rule", name = "top_body.0.call.17.121", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "drain"}} body {
      %c1_i32 = arith.constant 1 : i32 loc(#loc33)
      %0 = acir.ref @top::@top.output : !acir.queue<i32>
      %1 = acir.queue.peek %0 {acir.id = "v5", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc34)
      %2 = acir.ref @top::@top.last : !acir.cell<i32>
      acir.cell.write %2, %1 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc35)
      %3 = acir.ref @top::@top.retired : !acir.cell<i32>
      %4 = acir.cell.read %3 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc36)
      %5 = arith.addi %4, %c1_i32 : i32 loc(#loc36)
      acir.cell.write %3, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc37)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.18.125 : () -> () {acir.function = {contract = "Module:top", definition = "clock", instance = "top", interface = "Module:top:clock", kind = "rule", name = "top_body.0.call.18.125", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "clock"}} body {
      %c1_i32 = arith.constant 1 : i32 loc(#loc38)
      %0 = acir.ref @top::@top.cycle : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc39)
      %2 = arith.addi %1, %c1_i32 : i32 loc(#loc39)
      acir.cell.write %0, %2 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc40)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top::@top.cycle : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc41)
      %c0_i32 = arith.constant 0 : i32 loc(#loc42)
      %2 = arith.cmpi eq, %1, %c0_i32 : i32 loc(#loc41)
      %3 = acir.ref @top::@top.cycle : !acir.cell<i32>
      %4 = acir.ref @top::@top.cycle : !acir.cell<i32>
      scf.if %2 {
        %c2_i32_4 = arith.constant 2 : i32 loc(#loc43)
        %c20_i32 = arith.constant 20 : i32 loc(#loc44)
        %false = arith.constant false loc(#loc45)
        acir.rule.call @top::@top_body.0.call.0.8(%c2_i32_4, %c20_i32, %false) {acir.id = "v9", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : (i32, i32, i1) -> () loc(#loc46)
      } else {
        %20 = acir.cell.read %3 {acir.id = "v11", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc47)
        %c1_i32_4 = arith.constant 1 : i32 loc(#loc48)
        %21 = arith.cmpi eq, %20, %c1_i32_4 : i32 loc(#loc47)
        scf.if %21 {
          %c0_i32_5 = arith.constant 0 : i32 loc(#loc49)
          %c10_i32 = arith.constant 10 : i32 loc(#loc50)
          %true_6 = arith.constant true loc(#loc51)
          acir.rule.call @top::@top_body.0.call.0.8(%c0_i32_5, %c10_i32, %true_6) {acir.id = "v17", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : (i32, i32, i1) -> () loc(#loc52)
        } else {
          %22 = acir.cell.read %4 {acir.id = "v19", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc53)
          %c2_i32_5 = arith.constant 2 : i32 loc(#loc54)
          %23 = arith.cmpi eq, %22, %c2_i32_5 : i32 loc(#loc53)
          scf.if %23 {
            %c1_i32_6 = arith.constant 1 : i32 loc(#loc55)
            %c30_i32 = arith.constant 30 : i32 loc(#loc56)
            %true_7 = arith.constant true loc(#loc57)
            acir.rule.call @top::@top_body.0.call.0.8(%c1_i32_6, %c30_i32, %true_7) {acir.id = "v25", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : (i32, i32, i1) -> () loc(#loc58)
          } else {
          }
        }
      }
      %c0_i32_0 = arith.constant 0 : i32 loc(#loc59)
      %c0_i32_1 = arith.constant 0 : i32 loc(#loc59)
      %5 = acir.rule.call @top.slots.0::@top_body.0.call.6.36() {acir.id = "v37", acir.source_type = "src_array_arbiter_1242b928.Entry", contract = "Module:top.slots.0", instance = "top.slots.0", interface = "Module:top.slots.0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc60)
      %c1_i32 = arith.constant 1 : i32 loc(#loc59)
      %c1_i32_2 = arith.constant 1 : i32 loc(#loc59)
      %6 = acir.rule.call @top.slots.1::@top_body.0.call.7.39() {acir.id = "v40", acir.source_type = "src_array_arbiter_1242b928.Entry", contract = "Module:top.slots.1", instance = "top.slots.1", interface = "Module:top.slots.1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc60)
      %c2_i32 = arith.constant 2 : i32 loc(#loc59)
      %c2_i32_3 = arith.constant 2 : i32 loc(#loc59)
      %7 = acir.rule.call @top.slots.2::@top_body.0.call.8.42() {acir.id = "v43", acir.source_type = "src_array_arbiter_1242b928.Entry", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc60)
      %8 = acir.aggregate(%5, %6, %7) : (!acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> !acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]> loc(#loc61)
      %9 = acir.ref @top::@top.cycle : !acir.cell<i32>
      %10 = acir.cell.read %9 {acir.id = "v47", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc62)
      %c3_i32 = arith.constant 3 : i32 loc(#loc63)
      %11 = arith.cmpi uge, %10, %c3_i32 : i32 loc(#loc62)
      scf.if %11 {
        %20 = "acir.array.domain"(%8) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>) -> !acir.mask<[3]> loc(#loc64)
        %c0_i32_4 = arith.constant 0 : i32
        %21 = "acir.array.index"(%8, %c0_i32_4) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc64)
        %c0_i32_5 = arith.constant 0 : i32
        %22 = "acir.array.index"(%20, %c0_i32_5) : (!acir.mask<[3]>, i32) -> i1 loc(#loc64)
        %23 = scf.if %22 -> (i1) {
          %32 = acir.extract %21 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc65)
          %33 = scf.if %32 -> (i1) {
            %34 = acir.extract %21 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc66)
            scf.yield %34 : i1
          } else {
            %false = arith.constant false
            scf.yield %false : i1
          }
          scf.yield %33 : i1
        } else {
          %false = arith.constant false
          scf.yield %false : i1
        }
        %c1_i32_6 = arith.constant 1 : i32
        %24 = "acir.array.index"(%8, %c1_i32_6) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc64)
        %c1_i32_7 = arith.constant 1 : i32
        %25 = "acir.array.index"(%20, %c1_i32_7) : (!acir.mask<[3]>, i32) -> i1 loc(#loc64)
        %26 = scf.if %25 -> (i1) {
          %32 = acir.extract %24 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc65)
          %33 = scf.if %32 -> (i1) {
            %34 = acir.extract %24 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc66)
            scf.yield %34 : i1
          } else {
            %false = arith.constant false
            scf.yield %false : i1
          }
          scf.yield %33 : i1
        } else {
          %false = arith.constant false
          scf.yield %false : i1
        }
        %c2_i32_8 = arith.constant 2 : i32
        %27 = "acir.array.index"(%8, %c2_i32_8) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc64)
        %c2_i32_9 = arith.constant 2 : i32
        %28 = "acir.array.index"(%20, %c2_i32_9) : (!acir.mask<[3]>, i32) -> i1 loc(#loc64)
        %29 = scf.if %28 -> (i1) {
          %32 = acir.extract %27 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc65)
          %33 = scf.if %32 -> (i1) {
            %34 = acir.extract %27 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc66)
            scf.yield %34 : i1
          } else {
            %false = arith.constant false
            scf.yield %false : i1
          }
          scf.yield %33 : i1
        } else {
          %false = arith.constant false
          scf.yield %false : i1
        }
        %30 = acir.aggregate(%23, %26, %29) : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc64)
        %31 = "acir.array.select"(%8, %30) : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, !acir.mask<[3]>) -> !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>> loc(#loc67)
        acir.rule.call @top::@top_body.0.call.9.94(%31) {acir.id = "v95", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:issue", role = "rule", rule = "issue"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> () loc(#loc68)
      } else {
      }
      %12 = acir.ref @top::@top.cycle : !acir.cell<i32>
      %13 = acir.cell.read %12 {acir.id = "v100", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc69)
      %c5_i32 = arith.constant 5 : i32 loc(#loc70)
      %14 = arith.cmpi eq, %13, %c5_i32 : i32 loc(#loc69)
      scf.if %14 {
        %c2_i32_4 = arith.constant 2 : i32 loc(#loc71)
        acir.rule.call @top.slots.2::@top_body.0.call.16.103() {acir.id = "v104", acir.source_type = "void", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:wake", role = "rule", rule = "wake"} : () -> () loc(#loc72)
      } else {
      }
      %15 = acir.ref @top::@top.cycle : !acir.cell<i32>
      %16 = acir.cell.read %15 {acir.id = "v109", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc73)
      %c4_i32 = arith.constant 4 : i32 loc(#loc74)
      %17 = arith.cmpi uge, %16, %c4_i32 : i32 loc(#loc73)
      %18 = acir.ref @top::@top.cycle : !acir.cell<i32>
      %19 = scf.if %17 -> (i1) {
        %20 = acir.cell.read %18 {acir.id = "v113", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc75)
        %c2_i32_4 = arith.constant 2 : i32 loc(#loc76)
        %21 = func.call @ac_builtin_mod_u32(%20, %c2_i32_4) {acir.id = "v115", acir.source_type = "u32", operator = "%"} : (i32, i32) -> i32 loc(#loc75)
        %c0_i32_5 = arith.constant 0 : i32 loc(#loc77)
        %22 = arith.cmpi eq, %21, %c0_i32_5 : i32 loc(#loc75)
        scf.yield %22 : i1
      } else {
        %false = arith.constant false
        scf.yield %false : i1
      }
      scf.if %19 {
        acir.rule.call @top::@top_body.0.call.17.121() {acir.id = "v122", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:drain", role = "rule", rule = "drain"} : () -> () loc(#loc78)
      } else {
      }
      acir.rule.call @top::@top_body.0.call.18.125() {acir.id = "v126", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:clock", role = "rule", rule = "clock"} : () -> () loc(#loc79)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.slots.0 attributes {acir.instance = {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.0", parent = "top", rules = ["allocate", "peek", "take", "wake"]}} {
    acir.contract {relations = [], rules = ["allocate", "peek", "take", "wake"]}
    acir.cell @top.slots.0.state : !acir.struct<"src_array_arbiter_1242b928.Entry"> {initial_fn = "ac_init_top.slots.0.state", name = "top.slots.0.state", type = "src_array_arbiter_1242b928.Entry"}
    func.func private @ac_init_top.slots.0.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.1.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.2.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.age.used() -> !acir.array<i1, [3]>
    func.func private @ac_init_top.age.older() -> !acir.array<i1, [3, 3]>
    func.func private @ac_init_top.output_cap() -> i32
    func.func private @ac_init_top.output() -> tuple<i32>
    func.func private @ac_init_top.cycle() -> i32
    func.func private @ac_init_top.issued() -> i32
    func.func private @ac_init_top.last() -> i32
    func.func private @ac_init_top.retired() -> i32
    func.func private @ac_builtin_mod_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "mod", kind = "intrinsic", name = "ac_builtin_mod_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top_body.0.call.0.8.0.call.0.12.0.call.0.16 : (i32, i1) -> () {acir.function = {contract = "Module:top.slots.0", definition = "allocate", guard_info = {name = "", source = "variable"}, instance = "top.slots.0", interface = "Module:top.slots.0:allocate", kind = "rule", name = "top_body.0.call.0.8.0.call.0.12.0.call.0.16", output_type = "void", owner = "top.slots.0", params = [{id = "v1", name = "value", targets = [], type = "u32"}, {id = "v2", name = "ready", targets = [], type = "bool"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.0.16:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.0.16:ready")):
      %true = arith.constant true loc(#loc82)
      %0 = acir.ref @top.slots.0::@top.slots.0.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc83)
      %2 = acir.extract %1 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc83)
      %3 = arith.xori %2, %true : i1 loc(#loc82)
      acir.guard.yield %3 {acir.id = "v8", acir.source_type = "void"} loc(#loc82)
    } body {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.0.16:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.0.16:ready")):
      %true = arith.constant true
      %0 = acir.aggregate(%true, %arg1, %arg0) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc84)
      %1 = acir.ref @top.slots.0::@top.slots.0.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %1, %0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc85)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.6.36 : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.function = {contract = "Module:top.slots.0", definition = "peek", instance = "top.slots.0", interface = "Module:top.slots.0:peek", kind = "rule", name = "top_body.0.call.6.36", output_type = "src_array_arbiter_1242b928.Entry", owner = "top.slots.0", params = [], public = true, result = "src_array_arbiter_1242b928.Entry", rule = "peek"}} body {
      %0 = acir.ref @top.slots.0::@top.slots.0.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc86)
      acir.return %1 : !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.94.0.call.10.17.0.call.10.16 : () -> i32 {acir.function = {contract = "Module:top.slots.0", definition = "take", guard_info = {name = "", source = "variable"}, instance = "top.slots.0", interface = "Module:top.slots.0:take", kind = "rule", name = "top_body.0.call.9.94.0.call.10.17.0.call.10.16", output_type = "u32", owner = "top.slots.0", params = [], public = true, result = "u32", rule = "take"}} guard {
      %false = arith.constant false
      %0 = acir.ref @top.slots.0::@top.slots.0.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc87)
      %2 = acir.extract %1 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc87)
      %3 = scf.if %2 -> (i1) {
        %4 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc88)
        %5 = acir.extract %4 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc88)
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      acir.guard.yield %3 {acir.id = "v12", acir.source_type = "void"} loc(#loc87)
    } body {
      %c0_i32 = arith.constant 0 : i32 loc(#loc89)
      %false = arith.constant false
      %0 = acir.ref @top.slots.0::@top.slots.0.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v17", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc90)
      %2 = acir.extract %1 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc91)
      %3 = acir.aggregate(%false, %false, %c0_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc89)
      acir.cell.write %0, %3 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc92)
      acir.return %2 : i32 {acir.source_type = "void"}
    }
  }
  acir.module @top.slots.1 attributes {acir.instance = {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.1", parent = "top", rules = ["allocate", "peek", "take", "wake"]}} {
    acir.contract {relations = [], rules = ["allocate", "peek", "take", "wake"]}
    acir.cell @top.slots.1.state : !acir.struct<"src_array_arbiter_1242b928.Entry"> {initial_fn = "ac_init_top.slots.1.state", name = "top.slots.1.state", type = "src_array_arbiter_1242b928.Entry"}
    func.func private @ac_init_top.slots.0.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.1.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.2.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.age.used() -> !acir.array<i1, [3]>
    func.func private @ac_init_top.age.older() -> !acir.array<i1, [3, 3]>
    func.func private @ac_init_top.output_cap() -> i32
    func.func private @ac_init_top.output() -> tuple<i32>
    func.func private @ac_init_top.cycle() -> i32
    func.func private @ac_init_top.issued() -> i32
    func.func private @ac_init_top.last() -> i32
    func.func private @ac_init_top.retired() -> i32
    func.func private @ac_builtin_mod_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "mod", kind = "intrinsic", name = "ac_builtin_mod_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top_body.0.call.0.8.0.call.0.12.0.call.1.18 : (i32, i1) -> () {acir.function = {contract = "Module:top.slots.1", definition = "allocate", guard_info = {name = "", source = "variable"}, instance = "top.slots.1", interface = "Module:top.slots.1:allocate", kind = "rule", name = "top_body.0.call.0.8.0.call.0.12.0.call.1.18", output_type = "void", owner = "top.slots.1", params = [{id = "v1", name = "value", targets = [], type = "u32"}, {id = "v2", name = "ready", targets = [], type = "bool"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.1.18:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.1.18:ready")):
      %true = arith.constant true loc(#loc82)
      %0 = acir.ref @top.slots.1::@top.slots.1.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc83)
      %2 = acir.extract %1 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc83)
      %3 = arith.xori %2, %true : i1 loc(#loc82)
      acir.guard.yield %3 {acir.id = "v8", acir.source_type = "void"} loc(#loc82)
    } body {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.1.18:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.1.18:ready")):
      %true = arith.constant true
      %0 = acir.aggregate(%true, %arg1, %arg0) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc84)
      %1 = acir.ref @top.slots.1::@top.slots.1.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %1, %0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc85)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.7.39 : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.function = {contract = "Module:top.slots.1", definition = "peek", instance = "top.slots.1", interface = "Module:top.slots.1:peek", kind = "rule", name = "top_body.0.call.7.39", output_type = "src_array_arbiter_1242b928.Entry", owner = "top.slots.1", params = [], public = true, result = "src_array_arbiter_1242b928.Entry", rule = "peek"}} body {
      %0 = acir.ref @top.slots.1::@top.slots.1.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc86)
      acir.return %1 : !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.94.0.call.10.17.0.call.11.19 : () -> i32 {acir.function = {contract = "Module:top.slots.1", definition = "take", guard_info = {name = "", source = "variable"}, instance = "top.slots.1", interface = "Module:top.slots.1:take", kind = "rule", name = "top_body.0.call.9.94.0.call.10.17.0.call.11.19", output_type = "u32", owner = "top.slots.1", params = [], public = true, result = "u32", rule = "take"}} guard {
      %false = arith.constant false
      %0 = acir.ref @top.slots.1::@top.slots.1.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc87)
      %2 = acir.extract %1 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc87)
      %3 = scf.if %2 -> (i1) {
        %4 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc88)
        %5 = acir.extract %4 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc88)
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      acir.guard.yield %3 {acir.id = "v12", acir.source_type = "void"} loc(#loc87)
    } body {
      %c0_i32 = arith.constant 0 : i32 loc(#loc89)
      %false = arith.constant false
      %0 = acir.ref @top.slots.1::@top.slots.1.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v17", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc90)
      %2 = acir.extract %1 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc95)
      %3 = acir.aggregate(%false, %false, %c0_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc89)
      acir.cell.write %0, %3 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc92)
      acir.return %2 : i32 {acir.source_type = "void"}
    }
  }
  acir.module @top.slots.2 attributes {acir.instance = {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.2", parent = "top", rules = ["allocate", "peek", "take", "wake"]}} {
    acir.contract {relations = [], rules = ["allocate", "peek", "take", "wake"]}
    acir.cell @top.slots.2.state : !acir.struct<"src_array_arbiter_1242b928.Entry"> {initial_fn = "ac_init_top.slots.2.state", name = "top.slots.2.state", type = "src_array_arbiter_1242b928.Entry"}
    func.func private @ac_init_top.slots.0.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.1.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.2.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.age.used() -> !acir.array<i1, [3]>
    func.func private @ac_init_top.age.older() -> !acir.array<i1, [3, 3]>
    func.func private @ac_init_top.output_cap() -> i32
    func.func private @ac_init_top.output() -> tuple<i32>
    func.func private @ac_init_top.cycle() -> i32
    func.func private @ac_init_top.issued() -> i32
    func.func private @ac_init_top.last() -> i32
    func.func private @ac_init_top.retired() -> i32
    func.func private @ac_builtin_mod_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "mod", kind = "intrinsic", name = "ac_builtin_mod_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top_body.0.call.0.8.0.call.0.12.0.call.2.20 : (i32, i1) -> () {acir.function = {contract = "Module:top.slots.2", definition = "allocate", guard_info = {name = "", source = "variable"}, instance = "top.slots.2", interface = "Module:top.slots.2:allocate", kind = "rule", name = "top_body.0.call.0.8.0.call.0.12.0.call.2.20", output_type = "void", owner = "top.slots.2", params = [{id = "v1", name = "value", targets = [], type = "u32"}, {id = "v2", name = "ready", targets = [], type = "bool"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.2.20:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.2.20:ready")):
      %true = arith.constant true loc(#loc82)
      %0 = acir.ref @top.slots.2::@top.slots.2.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc83)
      %2 = acir.extract %1 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc83)
      %3 = arith.xori %2, %true : i1 loc(#loc82)
      acir.guard.yield %3 {acir.id = "v8", acir.source_type = "void"} loc(#loc82)
    } body {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.2.20:value"), %arg1: i1 loc("acir.binding:top_body.0.call.0.8.0.call.0.12.0.call.2.20:ready")):
      %true = arith.constant true
      %0 = acir.aggregate(%true, %arg1, %arg0) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc84)
      %1 = acir.ref @top.slots.2::@top.slots.2.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %1, %0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc85)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.8.42 : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.function = {contract = "Module:top.slots.2", definition = "peek", instance = "top.slots.2", interface = "Module:top.slots.2:peek", kind = "rule", name = "top_body.0.call.8.42", output_type = "src_array_arbiter_1242b928.Entry", owner = "top.slots.2", params = [], public = true, result = "src_array_arbiter_1242b928.Entry", rule = "peek"}} body {
      %0 = acir.ref @top.slots.2::@top.slots.2.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc86)
      acir.return %1 : !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.94.0.call.10.17.0.call.12.22 : () -> i32 {acir.function = {contract = "Module:top.slots.2", definition = "take", guard_info = {name = "", source = "variable"}, instance = "top.slots.2", interface = "Module:top.slots.2:take", kind = "rule", name = "top_body.0.call.9.94.0.call.10.17.0.call.12.22", output_type = "u32", owner = "top.slots.2", params = [], public = true, result = "u32", rule = "take"}} guard {
      %false = arith.constant false
      %0 = acir.ref @top.slots.2::@top.slots.2.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc87)
      %2 = acir.extract %1 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc87)
      %3 = scf.if %2 -> (i1) {
        %4 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc88)
        %5 = acir.extract %4 {field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc88)
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      acir.guard.yield %3 {acir.id = "v12", acir.source_type = "void"} loc(#loc87)
    } body {
      %c0_i32 = arith.constant 0 : i32 loc(#loc89)
      %false = arith.constant false
      %0 = acir.ref @top.slots.2::@top.slots.2.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v17", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc90)
      %2 = acir.extract %1 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc98)
      %3 = acir.aggregate(%false, %false, %c0_i32) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc89)
      acir.cell.write %0, %3 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc92)
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.16.103 : () -> () {acir.function = {contract = "Module:top.slots.2", definition = "wake", instance = "top.slots.2", interface = "Module:top.slots.2:wake", kind = "rule", name = "top_body.0.call.16.103", output_type = "void", owner = "top.slots.2", params = [], public = true, result = "void", rule = "wake"}} body {
      %true = arith.constant true
      %0 = acir.ref @top.slots.2::@top.slots.2.state : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc99)
      %2 = acir.extract %1 {field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc100)
      scf.if %2 {
        %3 = acir.extract %1 {field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc101)
        %4 = acir.aggregate(%2, %true, %3) : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc99)
        acir.cell.write %0, %4 {acir.id = "v16", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc102)
      } loc(#loc99)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.age attributes {acir.instance = {definition = "src_array_arbiter_1242b928.AgeMatrix", name = "top.age", parent = "top", rules = ["allocate", "choose", "release"]}} {
    acir.contract {relations = [], rules = ["allocate", "choose", "release"]}
    acir.cell @top.age.used : !acir.array<i1, [3]> {initial_fn = "ac_init_top.age.used", name = "top.age.used", type = "array<bool;3>"}
    acir.cell @top.age.older : !acir.array<i1, [3, 3]> {initial_fn = "ac_init_top.age.older", name = "top.age.older", type = "array<bool;3x3>"}
    func.func private @ac_init_top.slots.0.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.1.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.slots.2.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry">
    func.func private @ac_init_top.age.used() -> !acir.array<i1, [3]>
    func.func private @ac_init_top.age.older() -> !acir.array<i1, [3, 3]>
    func.func private @ac_init_top.output_cap() -> i32
    func.func private @ac_init_top.output() -> tuple<i32>
    func.func private @ac_init_top.cycle() -> i32
    func.func private @ac_init_top.issued() -> i32
    func.func private @ac_init_top.last() -> i32
    func.func private @ac_init_top.retired() -> i32
    func.func private @ac_builtin_mod_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "mod", kind = "intrinsic", name = "ac_builtin_mod_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top_body.0.call.0.8.0.call.4.13 : (i32) -> () {acir.function = {contract = "Module:top.age", definition = "allocate", instance = "top.age", interface = "Module:top.age:allocate", kind = "rule", name = "top_body.0.call.0.8.0.call.4.13", output_type = "void", owner = "top.age", params = [{id = "v1", name = "position", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.0.8.0.call.4.13:position")):
      %c2_i32 = arith.constant 2 : i32
      %c1_i32 = arith.constant 1 : i32
      %false = arith.constant false
      %c0_i32 = arith.constant 0 : i32
      %true = arith.constant true
      %0 = acir.ref @top.age::@top.age.used : !acir.cell<!acir.array<i1, [3]>>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "array<bool;3>", role = "observe"} : !acir.cell<!acir.array<i1, [3]>> -> !acir.array<i1, [3]> loc(#loc104)
      %2 = acir.ref @top.age::@top.age.older : !acir.cell<!acir.array<i1, [3, 3]>>
      %3 = acir.cell.read %2 {acir.id = "v9", acir.source_type = "array<bool;3x3>", role = "observe"} : !acir.cell<!acir.array<i1, [3, 3]>> -> !acir.array<i1, [3, 3]> loc(#loc105)
      %4 = "acir.array.index"(%1, %c0_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
      %5 = acir.update %3, %4[%c0_i32, %arg0] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc105)
      %6 = acir.update %5, %false[%arg0, %c0_i32] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc105)
      %7 = "acir.array.index"(%1, %c1_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
      %8 = acir.update %6, %7[%c1_i32, %arg0] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc105)
      %9 = acir.update %8, %false[%arg0, %c1_i32] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc105)
      %10 = "acir.array.index"(%1, %c2_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
      %11 = acir.update %9, %10[%c2_i32, %arg0] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc105)
      %12 = acir.update %11, %false[%arg0, %c2_i32] {path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc105)
      %13 = acir.update %1, %true[%arg0] {path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i32) -> !acir.array<i1, [3]> loc(#loc104)
      acir.cell.write %0, %13 {acir.id = "v42", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [3]>>, !acir.array<i1, [3]> loc(#loc107)
      acir.cell.write %2, %12 {acir.id = "v44", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [3, 3]>>, !acir.array<i1, [3, 3]> loc(#loc108)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.94.0.call.9.4 : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> tuple<i1, i32> {acir.function = {contract = "Module:top.age", definition = "choose", instance = "top.age", interface = "Module:top.age:choose", kind = "rule", name = "top_body.0.call.9.94.0.call.9.4", output_type = ["bool", "u32"], owner = "top.age", params = [{id = "v1", name = "candidates", targets = [], type = "selected<array<src_array_arbiter_1242b928.Entry;3>>"}], public = true, result = "tuple<bool,u32>", rule = "choose"}} body {
    ^bb0(%arg0: !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>> loc("acir.binding:top_body.0.call.9.94.0.call.9.4:candidates")):
      %c2_i32 = arith.constant 2 : i32
      %c1_i32 = arith.constant 1 : i32
      %c0_i32 = arith.constant 0 : i32
      %false = arith.constant false
      %true = arith.constant true
      %0 = "acir.array.domain"(%arg0) : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> !acir.mask<[3]> loc(#loc110)
      %1 = "acir.array.index"(%0, %c0_i32) : (!acir.mask<[3]>, i32) -> i1 loc(#loc110)
      %2 = "acir.array.index"(%0, %c1_i32) : (!acir.mask<[3]>, i32) -> i1 loc(#loc110)
      %3 = "acir.array.index"(%0, %c2_i32) : (!acir.mask<[3]>, i32) -> i1 loc(#loc110)
      %4 = acir.aggregate(%1, %2, %3) : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc111)
      %5 = acir.ref @top.age::@top.age.older : !acir.cell<!acir.array<i1, [3, 3]>>
      %6 = acir.cell.read %5 {acir.id = "v38", acir.source_type = "array<bool;3x3>", role = "observe"} : !acir.cell<!acir.array<i1, [3, 3]>> -> !acir.array<i1, [3, 3]> loc(#loc112)
      %7 = "acir.array.index"(%4, %c0_i32) {signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc113)
      %8 = scf.if %7 -> (i1) {
        %42 = "acir.array.index"(%6, %c0_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c0_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %9 = arith.xori %8, %true : i1 loc(#loc115)
      %10 = "acir.array.index"(%4, %c1_i32) {signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc113)
      %11 = scf.if %10 -> (i1) {
        %42 = "acir.array.index"(%6, %c1_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c0_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %12 = arith.select %11, %false, %9 : i1 loc(#loc115)
      %13 = "acir.array.index"(%4, %c2_i32) {signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc113)
      %14 = scf.if %13 -> (i1) {
        %42 = "acir.array.index"(%6, %c2_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c0_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %15 = arith.select %14, %false, %12 : i1 loc(#loc115)
      %16 = arith.select %7, %15, %false : i1
      %17 = acir.bind %16 : (i1) -> i1 loc(#loc116)
      %18 = acir.bind %c0_i32 : (i32) -> i32 loc(#loc117)
      %19 = scf.if %7 -> (i1) {
        %42 = "acir.array.index"(%6, %c0_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c1_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %20 = arith.xori %19, %true : i1 loc(#loc115)
      %21 = scf.if %10 -> (i1) {
        %42 = "acir.array.index"(%6, %c1_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c1_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %22 = arith.select %21, %false, %20 : i1 loc(#loc115)
      %23 = scf.if %13 -> (i1) {
        %42 = "acir.array.index"(%6, %c2_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c1_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %24 = arith.select %23, %false, %22 : i1 loc(#loc115)
      %25 = arith.select %10, %24, %false : i1
      %26 = arith.select %25, %true, %17 : i1
      %27 = arith.select %25, %c1_i32, %18 : i32
      %28 = acir.bind %26 : (i1) -> i1 loc(#loc116)
      %29 = acir.bind %27 : (i32) -> i32 loc(#loc117)
      %30 = scf.if %7 -> (i1) {
        %42 = "acir.array.index"(%6, %c0_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c2_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %31 = arith.xori %30, %true : i1 loc(#loc115)
      %32 = scf.if %10 -> (i1) {
        %42 = "acir.array.index"(%6, %c1_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c2_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %33 = arith.select %32, %false, %31 : i1 loc(#loc115)
      %34 = scf.if %13 -> (i1) {
        %42 = "acir.array.index"(%6, %c2_i32) {signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc114)
        %43 = "acir.array.index"(%42, %c2_i32) {signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc114)
        scf.yield %43 : i1
      } else {
        scf.yield %false : i1
      }
      %35 = arith.select %34, %false, %33 : i1 loc(#loc115)
      %36 = arith.select %13, %35, %false : i1
      %37 = arith.select %36, %true, %28 : i1
      %38 = arith.select %36, %c2_i32, %29 : i32
      %39 = acir.bind %37 : (i1) -> i1 loc(#loc116)
      %40 = acir.bind %38 : (i32) -> i32 loc(#loc117)
      %41 = acir.aggregate(%39, %40) : (i1, i32) -> tuple<i1, i32> loc(#loc118)
      acir.return %41 : tuple<i1, i32> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.94.0.call.14.19 : (i32) -> () {acir.function = {contract = "Module:top.age", definition = "release", instance = "top.age", interface = "Module:top.age:release", kind = "rule", name = "top_body.0.call.9.94.0.call.14.19", output_type = "void", owner = "top.age", params = [{id = "v1", name = "position", targets = [], type = "u32"}], public = true, result = "void", rule = "release"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top_body.0.call.9.94.0.call.14.19:position")):
      %false = arith.constant false loc(#loc120)
      %0 = acir.ref @top.age::@top.age.used : !acir.cell<!acir.array<i1, [3]>>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "array<bool;3>", role = "observe"} : !acir.cell<!acir.array<i1, [3]>> -> !acir.array<i1, [3]> loc(#loc121)
      %2 = acir.update %1, %false[%arg0] {path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i32) -> !acir.array<i1, [3]> loc(#loc121)
      acir.cell.write %0, %2 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [3]>>, !acir.array<i1, [3]> loc(#loc122)
      acir.return {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":18:40)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":44:39)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":45:42)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":45:41)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":85:43)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":86:41)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":87:42)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":88:40)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":89:43)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":93:9)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":94:9)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":100:21)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":98:27)
#loc26 = loc("acir.binding:top_body.0.call.9.94:found")
#loc27 = loc("acir.binding:top_body.0.call.9.94:position")
#loc28 = loc("acir.binding:top_body.0.call.9.94:value")
#loc29 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":101:13)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":102:13)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:22)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:13)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:29)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:16)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:9)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:19)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:9)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:25)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:17)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:9)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":115:12)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":115:21)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:22)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:25)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:29)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:13)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":117:14)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":117:23)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:22)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:25)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:29)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:13)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":119:14)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":119:23)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:22)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:25)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:29)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:13)
#loc59 = loc("-":121:35)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":121:29)
#loc61 = loc("acir.binding:top_body:entries")
#loc62 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":122:12)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":122:21)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:27)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:55)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:71)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:19)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:13)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":124:12)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":124:21)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":125:19)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":125:13)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:12)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:21)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:27)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:35)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:40)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":127:13)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":128:9)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:20)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:24)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:17)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:9)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":26:16)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:20)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:36)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:17)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":37:17)
#loc91 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17.0.call.10.16:value")
#loc92 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:9)
#loc95 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17.0.call.11.19:value")
#loc98 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17.0.call.12.22:value")
#loc99 = loc("acir.binding:top_body.0.call.16.103:entry")
#loc100 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":31:12)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":32:13)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":33:13)
#loc104 = loc("acir.binding:top_body.0.call.0.8.0.call.4.13:flags")
#loc105 = loc("acir.binding:top_body.0.call.0.8.0.call.4.13:matrix")
#loc106 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:35)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":55:9)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":56:9)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":66:16)
#loc111 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:mask")
#loc112 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:matrix")
#loc113 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:20)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:32)
#loc115 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:oldest")
#loc116 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:found")
#loc117 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:position")
#loc118 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":78:9)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":61:27)
#loc121 = loc("acir.binding:top_body.0.call.9.94.0.call.14.19:flags")
#loc122 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":62:9)

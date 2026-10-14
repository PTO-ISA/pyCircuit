module attributes {acir.model = {config = [], constants = {src_array_arbiter_1242b928.N = 3 : i64}, format_version = 9 : i64, top = "ArrayArbiter", type_order = ["src_array_arbiter_1242b928.Entry"], types = {src_array_arbiter_1242b928.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}} {
  func.func @ac_init_top.slots.0.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.slots.0.state", params = [], result = "src_array_arbiter_1242b928.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc1)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc1)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc1)
    %0 = acir.aggregate(%false, %false_0, %c0_i32) {acir.id = "v4", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc1)
    return %0 : !acir.struct<"src_array_arbiter_1242b928.Entry">
  }
  func.func @ac_init_top.slots.1.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.slots.1.state", params = [], result = "src_array_arbiter_1242b928.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc1)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc1)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc1)
    %0 = acir.aggregate(%false, %false_0, %c0_i32) {acir.id = "v9", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc1)
    return %0 : !acir.struct<"src_array_arbiter_1242b928.Entry">
  }
  func.func @ac_init_top.slots.2.state() -> !acir.struct<"src_array_arbiter_1242b928.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.slots.2.state", params = [], result = "src_array_arbiter_1242b928.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc1)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc1)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc1)
    %0 = acir.aggregate(%false, %false_0, %c0_i32) {acir.id = "v14", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc1)
    return %0 : !acir.struct<"src_array_arbiter_1242b928.Entry">
  }
  func.func @ac_init_top.age.used() -> !acir.array<i1, [3]> attributes {acir.function = {kind = "helper", name = "ac_init_top.age.used", params = [], result = "array<bool;3>"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc2)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc2)
    %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc2)
    %0 = acir.aggregate(%false, %false_0, %false_1) {acir.id = "v19", acir.source_type = "array<bool;3>"} : (i1, i1, i1) -> !acir.array<i1, [3]> loc(#loc3)
    return %0 : !acir.array<i1, [3]>
  }
  func.func @ac_init_top.age.older() -> !acir.array<i1, [3, 3]> attributes {acir.function = {kind = "helper", name = "ac_init_top.age.older", params = [], result = "array<bool;3x3>"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %0 = acir.aggregate(%false, %false_0, %false_1) {acir.id = "v24", acir.source_type = "array<bool;3>"} : (i1, i1, i1) -> !acir.array<i1, [3]> loc(#loc5)
    %false_2 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %false_3 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %false_4 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %1 = acir.aggregate(%false_2, %false_3, %false_4) {acir.id = "v28", acir.source_type = "array<bool;3>"} : (i1, i1, i1) -> !acir.array<i1, [3]> loc(#loc5)
    %false_5 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %false_6 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %false_7 = arith.constant {acir.source_type = "bool"} false loc(#loc4)
    %2 = acir.aggregate(%false_5, %false_6, %false_7) {acir.id = "v32", acir.source_type = "array<bool;3>"} : (i1, i1, i1) -> !acir.array<i1, [3]> loc(#loc5)
    %3 = acir.aggregate(%0, %1, %2) {acir.id = "v33", acir.source_type = "array<bool;3x3>"} : (!acir.array<i1, [3]>, !acir.array<i1, [3]>, !acir.array<i1, [3]>) -> !acir.array<i1, [3, 3]> loc(#loc6)
    return %3 : !acir.array<i1, [3, 3]>
  }
  func.func @ac_init_top.output_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.output_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.output() -> tuple<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.output", params = [], result = "tuple<u32>"}} {
    %c99_i32 = arith.constant {acir.source_type = "u32"} 99 : i32 loc(#loc7)
    %0 = acir.aggregate(%c99_i32) {acir.source_type = "tuple<u32>"} : (i32) -> tuple<i32>
    return %0 : tuple<i32>
  }
  func.func @ac_init_top.cycle() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cycle", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc8)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.issued() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.issued", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc9)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.last() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.last", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc10)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc11)
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
    ^bb0(%arg0: i32, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1):
      %0 = scf.if %arg2 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg3 : i1
      }
      %1 = scf.if %0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg4 : i1
      }
      acir.guard.yield %1 {acir.id = "v15", acir.source_type = "void"} loc(#loc12)
    } body {
    ^bb0(%arg0: i32, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = scf.if %arg2 -> (i1) {
        acir.rule.call @top.slots.0::@top_body.0.call.0.8.0.call.0.12.0.call.0.16(%arg0, %arg1) {acir.id = "v17", acir.source_type = "void", contract = "Module:top.slots.0", instance = "top.slots.0", interface = "Module:top.slots.0:allocate", role = "rule", rule = "allocate"} : (i32, i1) -> () loc(#loc12)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        %1 = scf.if %arg3 -> (i1) {
          acir.rule.call @top.slots.1::@top_body.0.call.0.8.0.call.0.12.0.call.1.18(%arg0, %arg1) {acir.id = "v19", acir.source_type = "void", contract = "Module:top.slots.1", instance = "top.slots.1", interface = "Module:top.slots.1:allocate", role = "rule", rule = "allocate"} : (i32, i1) -> () loc(#loc12)
          %false = arith.constant {acir.source_type = "bool"} false
          scf.yield %false : i1
        } else {
          %2 = scf.if %arg4 -> (i1) {
            acir.rule.call @top.slots.2::@top_body.0.call.0.8.0.call.0.12.0.call.2.20(%arg0, %arg1) {acir.id = "v21", acir.source_type = "void", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:allocate", role = "rule", rule = "allocate"} : (i32, i1) -> () loc(#loc12)
            %false = arith.constant {acir.source_type = "bool"} false
            scf.yield %false : i1
          } else {
            scf.yield %true : i1
          }
          scf.yield %2 : i1
        }
        scf.yield %1 : i1
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.0.8 : (i32, i32, i1) -> () {acir.function = {contract = "Module:top", definition = "allocate", instance = "top", interface = "Module:top:allocate", kind = "rule", name = "top_body.0.call.0.8", output_type = "void", owner = "top", params = [{id = "v1", name = "position", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}, {id = "v3", name = "ready", targets = [], type = "bool"}], public = true, result = "void", rule = "allocate"}} body {
    ^bb0(%arg0: i32, %arg1: i32, %arg2: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc12)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %1 = arith.cmpi eq, %arg0, %c1_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc12)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %2 = arith.cmpi eq, %arg0, %c2_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc12)
      acir.rule.call @top::@top_body.0.call.0.8.0.call.0.12(%arg1, %arg2, %0, %1, %2) {acir.id = "v13", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:$dispatch0", role = "rule", rule = "$dispatch0"} : (i32, i1, i1, i1, i1) -> () loc(#loc12)
      acir.rule.call @top.age::@top_body.0.call.0.8.0.call.4.13(%arg0) {acir.id = "v14", acir.source_type = "void", contract = "Module:top.age", instance = "top.age", interface = "Module:top.age:allocate", role = "rule", rule = "allocate"} : (i32) -> () loc(#loc13)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.93.0.call.10.15 : (i1, i1, i1) -> i32 {acir.function = {contract = "Module:top", definition = "$dispatch10", guard_info = {name = "", source = "variable"}, instance = "top", interface = "Module:top:$dispatch10", kind = "rule", name = "top_body.0.call.9.93.0.call.10.15", output_type = "u32", owner = "top", params = [{id = "v1", name = "$enable0", targets = [], type = "bool"}, {id = "v2", name = "$enable1", targets = [], type = "bool"}, {id = "v3", name = "$enable2", targets = [], type = "bool"}], public = true, result = "u32", rule = "$dispatch10"}} guard {
    ^bb0(%arg0: i1, %arg1: i1, %arg2: i1):
      %0 = scf.if %arg0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg1 : i1
      }
      %1 = scf.if %0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg2 : i1
      }
      acir.guard.yield %1 {acir.id = "v13", acir.source_type = "void"} loc(#loc14)
    } body {
    ^bb0(%arg0: i1, %arg1: i1, %arg2: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1:3 = scf.if %arg0 -> (i1, i1, i32) {
        %3 = acir.rule.call @top.slots.0::@top_body.0.call.9.93.0.call.10.15.0.call.10.16() {acir.id = "v17", acir.source_type = "u32", contract = "Module:top.slots.0", instance = "top.slots.0", interface = "Module:top.slots.0:take", role = "rule", rule = "take"} : () -> i32 loc(#loc14)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %3 : i1, i1, i32
      } else {
        %3:3 = scf.if %arg1 -> (i1, i1, i32) {
          %4 = acir.rule.call @top.slots.1::@top_body.0.call.9.93.0.call.10.15.0.call.11.19() {acir.id = "v20", acir.source_type = "u32", contract = "Module:top.slots.1", instance = "top.slots.1", interface = "Module:top.slots.1:take", role = "rule", rule = "take"} : () -> i32 loc(#loc14)
          %true_0 = arith.constant {acir.source_type = "bool"} true
          %false_1 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_1, %true_0, %4 : i1, i1, i32
        } else {
          %4:3 = scf.if %arg2 -> (i1, i1, i32) {
            %5 = acir.rule.call @top.slots.2::@top_body.0.call.9.93.0.call.10.15.0.call.12.22() {acir.id = "v23", acir.source_type = "u32", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:take", role = "rule", rule = "take"} : () -> i32 loc(#loc14)
            %true_0 = arith.constant {acir.source_type = "bool"} true
            %false_1 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_1, %true_0, %5 : i1, i1, i32
          } else {
            scf.yield %true, %false, %0 : i1, i1, i32
          }
          scf.yield %4#0, %4#1, %4#2 : i1, i1, i32
        }
        scf.yield %3#0, %3#1, %3#2 : i1, i1, i32
      }
      %2 = acir.unary %1#0 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %2 {acir.source_type = "void"}
      acir.return %1#2 : i32 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.93 : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> () {acir.function = {contract = "Module:top", definition = "issue", instance = "top", interface = "Module:top:issue", kind = "rule", name = "top_body.0.call.9.93", output_type = "void", owner = "top", params = [{id = "v1", name = "candidates", targets = [], type = "selected<array<src_array_arbiter_1242b928.Entry;3>>"}], public = true, result = "void", rule = "issue"}} body {
    ^bb0(%arg0: !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.rule.call @top.age::@top_body.0.call.9.93.0.call.9.4(%arg0) {acir.id = "v5", acir.source_type = "tuple<bool,u32>", contract = "Module:top.age", instance = "top.age", interface = "Module:top.age:choose", role = "rule", rule = "choose"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> tuple<i1, i32> loc(#loc15)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %1 = acir.extract %0[%c0_i32] {acir.id = "v7", acir.source_type = "bool", indexed = true, output_call = "v5", slot = 0 : i64} : (tuple<i1, i32>, i32) -> i1 loc(#loc15)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %2 = acir.extract %0[%c1_i32] {acir.id = "v9", acir.source_type = "u32", indexed = true, output_call = "v5", slot = 1 : i64} : (tuple<i1, i32>, i32) -> i32 loc(#loc15)
      %3 = acir.ref @top::@top.output {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %4 = acir.ref @top::@top.issued {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %5 = acir.ref @top::@top.issued {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      scf.if %1 {
        %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32
        %6 = arith.cmpi eq, %2, %c0_i32_0 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc14)
        %c1_i32_1 = arith.constant {acir.source_type = "u32"} 1 : i32
        %7 = arith.cmpi eq, %2, %c1_i32_1 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc14)
        %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
        %8 = arith.cmpi eq, %2, %c2_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc14)
        %9 = acir.rule.call @top::@top_body.0.call.9.93.0.call.10.15(%6, %7, %8) {acir.id = "v16", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:$dispatch10", role = "rule", rule = "$dispatch10"} : (i1, i1, i1) -> i32 loc(#loc14)
        acir.rule.call @top.age::@top_body.0.call.9.93.0.call.14.16(%2) {acir.id = "v17", acir.source_type = "void", contract = "Module:top.age", instance = "top.age", interface = "Module:top.age:release", role = "rule", rule = "release"} : (i32) -> () loc(#loc16)
        acir.queue.push %3, %9 {acir.id = "v19", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc17)
        %10 = acir.cell.read %4 {acir.id = "v21", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc18)
        %c1_i32_2 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc19)
        %11 = arith.addi %10, %c1_i32_2 {acir.id = "v23", acir.source_type = "u32"} : i32 loc(#loc18)
        acir.cell.write %5, %11 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc20)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.17.120 : () -> () {acir.function = {contract = "Module:top", definition = "drain", instance = "top", interface = "Module:top:drain", kind = "rule", name = "top_body.0.call.17.120", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "drain"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.output {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.queue.peek %0 {acir.id = "v5", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc21)
      %2 = acir.ref @top::@top.last {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %1 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc22)
      %3 = acir.ref @top::@top.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %4 = acir.cell.read %3 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc23)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc24)
      %5 = arith.addi %4, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc23)
      %6 = acir.ref @top::@top.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %6, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc25)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.18.124 : () -> () {acir.function = {contract = "Module:top", definition = "clock", instance = "top", interface = "Module:top:clock", kind = "rule", name = "top_body.0.call.18.124", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "clock"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc26)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc27)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc26)
      %3 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %3, %2 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc28)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc29)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc30)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc29)
      %3 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %4 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      scf.if %2 {
        %c2_i32_4 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc31)
        %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32 loc(#loc32)
        %false = arith.constant {acir.source_type = "bool"} false loc(#loc33)
        acir.rule.call @top::@top_body.0.call.0.8(%c2_i32_4, %c20_i32, %false) {acir.id = "v9", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : (i32, i32, i1) -> () loc(#loc34)
      } else {
        %20 = acir.cell.read %3 {acir.id = "v11", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc35)
        %c1_i32_4 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc36)
        %21 = arith.cmpi eq, %20, %c1_i32_4 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc35)
        scf.if %21 {
          %c0_i32_5 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc37)
          %c10_i32 = arith.constant {acir.source_type = "u32"} 10 : i32 loc(#loc38)
          %true_6 = arith.constant {acir.source_type = "bool"} true loc(#loc39)
          acir.rule.call @top::@top_body.0.call.0.8(%c0_i32_5, %c10_i32, %true_6) {acir.id = "v17", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : (i32, i32, i1) -> () loc(#loc40)
        } else {
          %22 = acir.cell.read %4 {acir.id = "v19", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc41)
          %c2_i32_5 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc42)
          %23 = arith.cmpi eq, %22, %c2_i32_5 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc41)
          scf.if %23 {
            %c1_i32_6 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc43)
            %c30_i32 = arith.constant {acir.source_type = "u32"} 30 : i32 loc(#loc44)
            %true_7 = arith.constant {acir.source_type = "bool"} true loc(#loc45)
            acir.rule.call @top::@top_body.0.call.0.8(%c1_i32_6, %c30_i32, %true_7) {acir.id = "v25", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : (i32, i32, i1) -> () loc(#loc46)
          } else {
          }
        }
      }
      %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc47)
      %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc47)
      %5 = acir.rule.call @top.slots.0::@top_body.0.call.6.36() {acir.id = "v37", acir.source_type = "src_array_arbiter_1242b928.Entry", contract = "Module:top.slots.0", instance = "top.slots.0", interface = "Module:top.slots.0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc48)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc47)
      %c1_i32_2 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc47)
      %6 = acir.rule.call @top.slots.1::@top_body.0.call.7.39() {acir.id = "v40", acir.source_type = "src_array_arbiter_1242b928.Entry", contract = "Module:top.slots.1", instance = "top.slots.1", interface = "Module:top.slots.1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc48)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc47)
      %c2_i32_3 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc47)
      %7 = acir.rule.call @top.slots.2::@top_body.0.call.8.42() {acir.id = "v43", acir.source_type = "src_array_arbiter_1242b928.Entry", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc48)
      %8 = acir.aggregate(%5, %6, %7) {acir.id = "v44", acir.source_type = "array<src_array_arbiter_1242b928.Entry;3>"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">, !acir.struct<"src_array_arbiter_1242b928.Entry">) -> !acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]> loc(#loc49)
      %9 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %10 = acir.cell.read %9 {acir.id = "v46", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc50)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc51)
      %11 = arith.cmpi uge, %10, %c3_i32 {acir.id = "v48", acir.source_type = "bool"} : i32 loc(#loc50)
      scf.if %11 {
        %20 = "acir.array.domain"(%8) {acir.id = "v49", acir.source_type = "mask<3>"} : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>) -> !acir.mask<[3]> loc(#loc52)
        %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32
        %21 = "acir.array.index"(%8, %c0_i32_4) {acir.id = "v51", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc52)
        %c0_i32_5 = arith.constant {acir.source_type = "u32"} 0 : i32
        %22 = "acir.array.index"(%20, %c0_i32_5) {acir.id = "v53", acir.source_type = "bool"} : (!acir.mask<[3]>, i32) -> i1 loc(#loc52)
        %23 = scf.if %22 -> (i1) {
          %32 = acir.extract %21 {acir.id = "v54", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc53)
          %33 = scf.if %32 -> (i1) {
            %34 = acir.extract %21 {acir.id = "v55", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc54)
            scf.yield %34 : i1
          } else {
            %false = arith.constant {acir.source_type = "bool"} false
            scf.yield %false : i1
          }
          scf.yield %33 : i1
        } else {
          %false = arith.constant {acir.source_type = "bool"} false
          scf.yield %false : i1
        }
        %c1_i32_6 = arith.constant {acir.source_type = "u32"} 1 : i32
        %24 = "acir.array.index"(%8, %c1_i32_6) {acir.id = "v65", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc52)
        %c1_i32_7 = arith.constant {acir.source_type = "u32"} 1 : i32
        %25 = "acir.array.index"(%20, %c1_i32_7) {acir.id = "v67", acir.source_type = "bool"} : (!acir.mask<[3]>, i32) -> i1 loc(#loc52)
        %26 = scf.if %25 -> (i1) {
          %32 = acir.extract %24 {acir.id = "v68", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc53)
          %33 = scf.if %32 -> (i1) {
            %34 = acir.extract %24 {acir.id = "v69", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc54)
            scf.yield %34 : i1
          } else {
            %false = arith.constant {acir.source_type = "bool"} false
            scf.yield %false : i1
          }
          scf.yield %33 : i1
        } else {
          %false = arith.constant {acir.source_type = "bool"} false
          scf.yield %false : i1
        }
        %c2_i32_8 = arith.constant {acir.source_type = "u32"} 2 : i32
        %27 = "acir.array.index"(%8, %c2_i32_8) {acir.id = "v79", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc52)
        %c2_i32_9 = arith.constant {acir.source_type = "u32"} 2 : i32
        %28 = "acir.array.index"(%20, %c2_i32_9) {acir.id = "v81", acir.source_type = "bool"} : (!acir.mask<[3]>, i32) -> i1 loc(#loc52)
        %29 = scf.if %28 -> (i1) {
          %32 = acir.extract %27 {acir.id = "v82", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc53)
          %33 = scf.if %32 -> (i1) {
            %34 = acir.extract %27 {acir.id = "v83", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc54)
            scf.yield %34 : i1
          } else {
            %false = arith.constant {acir.source_type = "bool"} false
            scf.yield %false : i1
          }
          scf.yield %33 : i1
        } else {
          %false = arith.constant {acir.source_type = "bool"} false
          scf.yield %false : i1
        }
        %30 = acir.aggregate(%23, %26, %29) {acir.id = "v92", acir.source_type = "mask<3>"} : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc52)
        %31 = "acir.array.select"(%8, %30) {acir.id = "v93", acir.source_type = "selected<array<src_array_arbiter_1242b928.Entry;3>>"} : (!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>, !acir.mask<[3]>) -> !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>> loc(#loc55)
        acir.rule.call @top::@top_body.0.call.9.93(%31) {acir.id = "v94", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:issue", role = "rule", rule = "issue"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> () loc(#loc56)
      } else {
      }
      %12 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %13 = acir.cell.read %12 {acir.id = "v99", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc57)
      %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32 loc(#loc58)
      %14 = arith.cmpi eq, %13, %c5_i32 {acir.id = "v101", acir.source_type = "bool"} : i32 loc(#loc57)
      scf.if %14 {
        %c2_i32_4 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc59)
        acir.rule.call @top.slots.2::@top_body.0.call.16.102() {acir.id = "v103", acir.source_type = "void", contract = "Module:top.slots.2", instance = "top.slots.2", interface = "Module:top.slots.2:wake", role = "rule", rule = "wake"} : () -> () loc(#loc60)
      } else {
      }
      %15 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %16 = acir.cell.read %15 {acir.id = "v108", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc61)
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc62)
      %17 = arith.cmpi uge, %16, %c4_i32 {acir.id = "v110", acir.source_type = "bool"} : i32 loc(#loc61)
      %18 = acir.ref @top::@top.cycle {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %19 = scf.if %17 -> (i1) {
        %20 = acir.cell.read %18 {acir.id = "v112", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc63)
        %c2_i32_4 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc64)
        %21 = func.call @ac_builtin_mod_u32(%20, %c2_i32_4) {acir.id = "v114", acir.source_type = "u32", operator = "%"} : (i32, i32) -> i32 loc(#loc63)
        %c0_i32_5 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc65)
        %22 = arith.cmpi eq, %21, %c0_i32_5 {acir.id = "v116", acir.source_type = "bool"} : i32 loc(#loc63)
        scf.yield %22 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      scf.if %19 {
        acir.rule.call @top::@top_body.0.call.17.120() {acir.id = "v121", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:drain", role = "rule", rule = "drain"} : () -> () loc(#loc66)
      } else {
      }
      acir.rule.call @top::@top_body.0.call.18.124() {acir.id = "v125", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:clock", role = "rule", rule = "clock"} : () -> () loc(#loc67)
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
    ^bb0(%arg0: i32, %arg1: i1):
      %0 = acir.ref @top.slots.0::@top.slots.0.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc68)
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc68)
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc69)
      acir.guard.yield %3 {acir.id = "v8", acir.source_type = "void"} loc(#loc69)
    } body {
    ^bb0(%arg0: i32, %arg1: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc70)
      %0 = acir.aggregate(%true_0, %arg1, %arg0) {acir.id = "v11", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc71)
      %1 = acir.ref @top.slots.0::@top.slots.0.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %1, %0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc72)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.6.36 : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.function = {contract = "Module:top.slots.0", definition = "peek", instance = "top.slots.0", interface = "Module:top.slots.0:peek", kind = "rule", name = "top_body.0.call.6.36", output_type = "src_array_arbiter_1242b928.Entry", owner = "top.slots.0", params = [], public = true, result = "src_array_arbiter_1242b928.Entry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_array_arbiter_1242b928.Entry"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.slots.0::@top.slots.0.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc73)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.93.0.call.10.15.0.call.10.16 : () -> i32 {acir.function = {contract = "Module:top.slots.0", definition = "take", guard_info = {name = "", source = "variable"}, instance = "top.slots.0", interface = "Module:top.slots.0:take", kind = "rule", name = "top_body.0.call.9.93.0.call.10.15.0.call.10.16", output_type = "u32", owner = "top.slots.0", params = [], public = true, result = "u32", rule = "take"}} guard {
      %0 = acir.ref @top.slots.0::@top.slots.0.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc74)
      %2 = acir.extract %1 {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc74)
      %3 = acir.ref @top.slots.0::@top.slots.0.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %4 = scf.if %2 -> (i1) {
        %5 = acir.cell.read %3 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc75)
        %6 = acir.extract %5 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc75)
        scf.yield %6 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      acir.guard.yield %4 {acir.id = "v12", acir.source_type = "void"} loc(#loc74)
    } body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.slots.0::@top.slots.0.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v17", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc76)
      %3 = acir.extract %2 {acir.id = "v18", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc76)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc77)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc77)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc77)
      %4 = acir.aggregate(%false_0, %false_1, %c0_i32) {acir.id = "v22", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc77)
      %5 = acir.ref @top.slots.0::@top.slots.0.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %5, %4 {acir.id = "v24", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc78)
      %true_2 = arith.constant {acir.source_type = "bool"} true
      %false_3 = arith.constant {acir.source_type = "bool"} false
      %6 = acir.unary %false_3 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %3 : i32 {acir.source_type = "void"}
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
    ^bb0(%arg0: i32, %arg1: i1):
      %0 = acir.ref @top.slots.1::@top.slots.1.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc68)
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc68)
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc69)
      acir.guard.yield %3 {acir.id = "v8", acir.source_type = "void"} loc(#loc69)
    } body {
    ^bb0(%arg0: i32, %arg1: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc70)
      %0 = acir.aggregate(%true_0, %arg1, %arg0) {acir.id = "v11", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc71)
      %1 = acir.ref @top.slots.1::@top.slots.1.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %1, %0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc72)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.7.39 : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.function = {contract = "Module:top.slots.1", definition = "peek", instance = "top.slots.1", interface = "Module:top.slots.1:peek", kind = "rule", name = "top_body.0.call.7.39", output_type = "src_array_arbiter_1242b928.Entry", owner = "top.slots.1", params = [], public = true, result = "src_array_arbiter_1242b928.Entry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_array_arbiter_1242b928.Entry"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.slots.1::@top.slots.1.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc73)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.93.0.call.10.15.0.call.11.19 : () -> i32 {acir.function = {contract = "Module:top.slots.1", definition = "take", guard_info = {name = "", source = "variable"}, instance = "top.slots.1", interface = "Module:top.slots.1:take", kind = "rule", name = "top_body.0.call.9.93.0.call.10.15.0.call.11.19", output_type = "u32", owner = "top.slots.1", params = [], public = true, result = "u32", rule = "take"}} guard {
      %0 = acir.ref @top.slots.1::@top.slots.1.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc74)
      %2 = acir.extract %1 {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc74)
      %3 = acir.ref @top.slots.1::@top.slots.1.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %4 = scf.if %2 -> (i1) {
        %5 = acir.cell.read %3 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc75)
        %6 = acir.extract %5 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc75)
        scf.yield %6 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      acir.guard.yield %4 {acir.id = "v12", acir.source_type = "void"} loc(#loc74)
    } body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.slots.1::@top.slots.1.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v17", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc76)
      %3 = acir.extract %2 {acir.id = "v18", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc76)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc77)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc77)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc77)
      %4 = acir.aggregate(%false_0, %false_1, %c0_i32) {acir.id = "v22", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc77)
      %5 = acir.ref @top.slots.1::@top.slots.1.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %5, %4 {acir.id = "v24", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc78)
      %true_2 = arith.constant {acir.source_type = "bool"} true
      %false_3 = arith.constant {acir.source_type = "bool"} false
      %6 = acir.unary %false_3 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %3 : i32 {acir.source_type = "void"}
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
    ^bb0(%arg0: i32, %arg1: i1):
      %0 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc68)
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc68)
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc69)
      acir.guard.yield %3 {acir.id = "v8", acir.source_type = "void"} loc(#loc69)
    } body {
    ^bb0(%arg0: i32, %arg1: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc70)
      %0 = acir.aggregate(%true_0, %arg1, %arg0) {acir.id = "v11", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc71)
      %1 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %1, %0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc72)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.8.42 : () -> !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.function = {contract = "Module:top.slots.2", definition = "peek", instance = "top.slots.2", interface = "Module:top.slots.2:peek", kind = "rule", name = "top_body.0.call.8.42", output_type = "src_array_arbiter_1242b928.Entry", owner = "top.slots.2", params = [], public = true, result = "src_array_arbiter_1242b928.Entry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_array_arbiter_1242b928.Entry"} : () -> !acir.struct<"src_array_arbiter_1242b928.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc73)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_array_arbiter_1242b928.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.93.0.call.10.15.0.call.12.22 : () -> i32 {acir.function = {contract = "Module:top.slots.2", definition = "take", guard_info = {name = "", source = "variable"}, instance = "top.slots.2", interface = "Module:top.slots.2:take", kind = "rule", name = "top_body.0.call.9.93.0.call.10.15.0.call.12.22", output_type = "u32", owner = "top.slots.2", params = [], public = true, result = "u32", rule = "take"}} guard {
      %0 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc74)
      %2 = acir.extract %1 {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc74)
      %3 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %4 = scf.if %2 -> (i1) {
        %5 = acir.cell.read %3 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc75)
        %6 = acir.extract %5 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc75)
        scf.yield %6 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      acir.guard.yield %4 {acir.id = "v12", acir.source_type = "void"} loc(#loc74)
    } body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v17", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc76)
      %3 = acir.extract %2 {acir.id = "v18", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc76)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc77)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc77)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc77)
      %4 = acir.aggregate(%false_0, %false_1, %c0_i32) {acir.id = "v22", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc77)
      %5 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      acir.cell.write %5, %4 {acir.id = "v24", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc78)
      %true_2 = arith.constant {acir.source_type = "bool"} true
      %false_3 = arith.constant {acir.source_type = "bool"} false
      %6 = acir.unary %false_3 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %3 : i32 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.16.102 : () -> () {acir.function = {contract = "Module:top.slots.2", definition = "wake", instance = "top.slots.2", interface = "Module:top.slots.2:wake", kind = "rule", name = "top_body.0.call.16.102", output_type = "void", owner = "top.slots.2", params = [], public = true, result = "void", rule = "wake"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "src_array_arbiter_1242b928.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">> -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc79)
      %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc80)
      %3 = acir.ref @top.slots.2::@top.slots.2.state {acir.source_type = "cell<src_array_arbiter_1242b928.Entry>"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>
      %4 = scf.if %2 -> (!acir.struct<"src_array_arbiter_1242b928.Entry">) {
        %5 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc81)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc82)
        %6 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc81)
        %7 = acir.extract %1 {acir.id = "v10", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i1 loc(#loc81)
        %8 = acir.extract %1 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_array_arbiter_1242b928.Entry">) -> i32 loc(#loc81)
        %9 = acir.aggregate(%6, %true_0, %8) {acir.id = "v12", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (i1, i1, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc81)
        acir.cell.write %3, %9 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_array_arbiter_1242b928.Entry">>, !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc83)
        scf.yield %9 : !acir.struct<"src_array_arbiter_1242b928.Entry">
      } else {
        scf.yield %1 : !acir.struct<"src_array_arbiter_1242b928.Entry">
      }
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
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.age::@top.age.used {acir.source_type = "cell<array<bool;3>>"} : !acir.cell<!acir.array<i1, [3]>>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "array<bool;3>", role = "observe"} : !acir.cell<!acir.array<i1, [3]>> -> !acir.array<i1, [3]> loc(#loc84)
      %2 = acir.ref @top.age::@top.age.older {acir.source_type = "cell<array<bool;3x3>>"} : !acir.cell<!acir.array<i1, [3, 3]>>
      %3 = acir.cell.read %2 {acir.id = "v8", acir.source_type = "array<bool;3x3>", role = "observe"} : !acir.cell<!acir.array<i1, [3, 3]>> -> !acir.array<i1, [3, 3]> loc(#loc85)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc86)
      %4 = "acir.array.index"(%1, %c0_i32) {acir.id = "v10", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc87)
      %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc88)
      %5 = acir.update %3, %4[%c0_i32_0, %arg0] {acir.id = "v12", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc89)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc90)
      %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc91)
      %6 = acir.update %5, %false[%arg0, %c0_i32_1] {acir.id = "v15", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc92)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc86)
      %7 = "acir.array.index"(%1, %c1_i32) {acir.id = "v17", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc87)
      %c1_i32_2 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc88)
      %8 = acir.update %6, %7[%c1_i32_2, %arg0] {acir.id = "v19", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc89)
      %false_3 = arith.constant {acir.source_type = "bool"} false loc(#loc90)
      %c1_i32_4 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc91)
      %9 = acir.update %8, %false_3[%arg0, %c1_i32_4] {acir.id = "v22", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc92)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc86)
      %10 = "acir.array.index"(%1, %c2_i32) {acir.id = "v24", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc87)
      %c2_i32_5 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc88)
      %11 = acir.update %9, %10[%c2_i32_5, %arg0] {acir.id = "v26", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc89)
      %false_6 = arith.constant {acir.source_type = "bool"} false loc(#loc90)
      %c2_i32_7 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc91)
      %12 = acir.update %11, %false_6[%arg0, %c2_i32_7] {acir.id = "v29", acir.source_type = "array<bool;3x3>", path = [unit, unit], signed_indices = [false, false]} : (!acir.array<i1, [3, 3]>, i1, i32, i32) -> !acir.array<i1, [3, 3]> loc(#loc92)
      %true_8 = arith.constant {acir.source_type = "bool"} true loc(#loc93)
      %13 = acir.update %1, %true_8[%arg0] {acir.id = "v31", acir.source_type = "array<bool;3>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i32) -> !acir.array<i1, [3]> loc(#loc94)
      %14 = acir.ref @top.age::@top.age.used {acir.source_type = "cell<array<bool;3>>"} : !acir.cell<!acir.array<i1, [3]>>
      acir.cell.write %14, %13 {acir.id = "v33", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [3]>>, !acir.array<i1, [3]> loc(#loc95)
      %15 = acir.ref @top.age::@top.age.older {acir.source_type = "cell<array<bool;3x3>>"} : !acir.cell<!acir.array<i1, [3, 3]>>
      acir.cell.write %15, %12 {acir.id = "v35", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [3, 3]>>, !acir.array<i1, [3, 3]> loc(#loc96)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.93.0.call.9.4 : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> tuple<i1, i32> {acir.function = {contract = "Module:top.age", definition = "choose", instance = "top.age", interface = "Module:top.age:choose", kind = "rule", name = "top_body.0.call.9.93.0.call.9.4", output_type = ["bool", "u32"], owner = "top.age", params = [{id = "v1", name = "candidates", targets = [], type = "selected<array<src_array_arbiter_1242b928.Entry;3>>"}], public = true, result = "tuple<bool,u32>", rule = "choose"}} body {
    ^bb0(%arg0: !acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "tuple<bool,u32>"} : () -> tuple<i1, i32>
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = "acir.array.domain"(%arg0) {acir.id = "v7", acir.source_type = "mask<3>"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>) -> !acir.mask<[3]> loc(#loc97)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %2 = "acir.array.index"(%arg0, %c0_i32) {acir.id = "v9", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc97)
      %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32
      %3 = "acir.array.index"(%1, %c0_i32_0) {acir.id = "v11", acir.source_type = "bool"} : (!acir.mask<[3]>, i32) -> i1 loc(#loc97)
      %4 = scf.if %3 -> (i1) {
        %true_24 = arith.constant {acir.source_type = "bool"} true loc(#loc98)
        scf.yield %true_24 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %5 = "acir.array.index"(%arg0, %c1_i32) {acir.id = "v18", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc97)
      %c1_i32_1 = arith.constant {acir.source_type = "u32"} 1 : i32
      %6 = "acir.array.index"(%1, %c1_i32_1) {acir.id = "v20", acir.source_type = "bool"} : (!acir.mask<[3]>, i32) -> i1 loc(#loc97)
      %7 = scf.if %6 -> (i1) {
        %true_24 = arith.constant {acir.source_type = "bool"} true loc(#loc98)
        scf.yield %true_24 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %8 = "acir.array.index"(%arg0, %c2_i32) {acir.id = "v27", acir.source_type = "src_array_arbiter_1242b928.Entry"} : (!acir.selected<!acir.array<!acir.struct<"src_array_arbiter_1242b928.Entry">, [3]>>, i32) -> !acir.struct<"src_array_arbiter_1242b928.Entry"> loc(#loc97)
      %c2_i32_2 = arith.constant {acir.source_type = "u32"} 2 : i32
      %9 = "acir.array.index"(%1, %c2_i32_2) {acir.id = "v29", acir.source_type = "bool"} : (!acir.mask<[3]>, i32) -> i1 loc(#loc97)
      %10 = scf.if %9 -> (i1) {
        %true_24 = arith.constant {acir.source_type = "bool"} true loc(#loc98)
        scf.yield %true_24 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %11 = acir.aggregate(%4, %7, %10) {acir.id = "v35", acir.source_type = "mask<3>"} : (i1, i1, i1) -> !acir.mask<[3]> loc(#loc97)
      %12 = acir.ref @top.age::@top.age.older {acir.source_type = "cell<array<bool;3x3>>"} : !acir.cell<!acir.array<i1, [3, 3]>>
      %13 = acir.cell.read %12 {acir.id = "v37", acir.source_type = "array<bool;3x3>", role = "observe"} : !acir.cell<!acir.array<i1, [3, 3]>> -> !acir.array<i1, [3, 3]> loc(#loc99)
      %false_3 = arith.constant {acir.source_type = "bool"} false loc(#loc100)
      %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc101)
      %true_5 = arith.constant {acir.source_type = "bool"} true loc(#loc102)
      %c0_i32_6 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc103)
      %14 = "acir.array.index"(%11, %c0_i32_6) {acir.id = "v42", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %15 = scf.if %14 -> (i1) {
        %c0_i32_24 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c0_i32_24) {acir.id = "v44", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c0_i32_25 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c0_i32_25) {acir.id = "v46", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %16 = scf.if %15 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %true_5 : i1
      }
      %c1_i32_7 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc103)
      %17 = "acir.array.index"(%11, %c1_i32_7) {acir.id = "v57", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %18 = scf.if %17 -> (i1) {
        %c1_i32_24 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c1_i32_24) {acir.id = "v59", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c0_i32_25 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c0_i32_25) {acir.id = "v61", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %19 = scf.if %18 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %16 : i1
      }
      %c2_i32_8 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc103)
      %20 = "acir.array.index"(%11, %c2_i32_8) {acir.id = "v72", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %21 = scf.if %20 -> (i1) {
        %c2_i32_24 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c2_i32_24) {acir.id = "v74", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c0_i32_25 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c0_i32_25) {acir.id = "v76", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %22 = scf.if %21 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %19 : i1
      }
      %c0_i32_9 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc109)
      %23 = "acir.array.index"(%11, %c0_i32_9) {acir.id = "v87", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc110)
      %24 = scf.if %23 -> (i1) {
        scf.yield %22 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %25:2 = scf.if %24 -> (i1, i32) {
        %true_24 = arith.constant {acir.source_type = "bool"} true loc(#loc111)
        %c0_i32_25 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc112)
        scf.yield %true_24, %c0_i32_25 : i1, i32
      } else {
        scf.yield %false_3, %c0_i32_4 : i1, i32
      }
      %true_10 = arith.constant {acir.source_type = "bool"} true loc(#loc102)
      %c0_i32_11 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc103)
      %26 = "acir.array.index"(%11, %c0_i32_11) {acir.id = "v101", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %27 = scf.if %26 -> (i1) {
        %c0_i32_24 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c0_i32_24) {acir.id = "v103", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c1_i32_25 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c1_i32_25) {acir.id = "v105", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %28 = scf.if %27 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %true_10 : i1
      }
      %c1_i32_12 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc103)
      %29 = "acir.array.index"(%11, %c1_i32_12) {acir.id = "v116", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %30 = scf.if %29 -> (i1) {
        %c1_i32_24 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c1_i32_24) {acir.id = "v118", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c1_i32_25 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c1_i32_25) {acir.id = "v120", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %31 = scf.if %30 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %28 : i1
      }
      %c2_i32_13 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc103)
      %32 = "acir.array.index"(%11, %c2_i32_13) {acir.id = "v131", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %33 = scf.if %32 -> (i1) {
        %c2_i32_24 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c2_i32_24) {acir.id = "v133", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c1_i32_25 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c1_i32_25) {acir.id = "v135", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %34 = scf.if %33 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %31 : i1
      }
      %c1_i32_14 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc109)
      %35 = "acir.array.index"(%11, %c1_i32_14) {acir.id = "v146", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc110)
      %36 = scf.if %35 -> (i1) {
        scf.yield %34 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %37:2 = scf.if %36 -> (i1, i32) {
        %true_24 = arith.constant {acir.source_type = "bool"} true loc(#loc111)
        %c1_i32_25 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc112)
        scf.yield %true_24, %c1_i32_25 : i1, i32
      } else {
        scf.yield %25#0, %25#1 : i1, i32
      }
      %true_15 = arith.constant {acir.source_type = "bool"} true loc(#loc102)
      %c0_i32_16 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc103)
      %38 = "acir.array.index"(%11, %c0_i32_16) {acir.id = "v160", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %39 = scf.if %38 -> (i1) {
        %c0_i32_24 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c0_i32_24) {acir.id = "v162", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c2_i32_25 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c2_i32_25) {acir.id = "v164", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %40 = scf.if %39 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %true_15 : i1
      }
      %c1_i32_17 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc103)
      %41 = "acir.array.index"(%11, %c1_i32_17) {acir.id = "v175", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %42 = scf.if %41 -> (i1) {
        %c1_i32_24 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c1_i32_24) {acir.id = "v177", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c2_i32_25 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c2_i32_25) {acir.id = "v179", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %43 = scf.if %42 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %40 : i1
      }
      %c2_i32_18 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc103)
      %44 = "acir.array.index"(%11, %c2_i32_18) {acir.id = "v190", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc104)
      %45 = scf.if %44 -> (i1) {
        %c2_i32_24 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc105)
        %52 = "acir.array.index"(%13, %c2_i32_24) {acir.id = "v192", acir.source_type = "array<bool;3>", signed_index = false} : (!acir.array<i1, [3, 3]>, i32) -> !acir.array<i1, [3]> loc(#loc106)
        %c2_i32_25 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc107)
        %53 = "acir.array.index"(%52, %c2_i32_25) {acir.id = "v194", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [3]>, i32) -> i1 loc(#loc106)
        scf.yield %53 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %46 = scf.if %45 -> (i1) {
        %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc108)
        scf.yield %false_24 : i1
      } else {
        scf.yield %43 : i1
      }
      %c2_i32_19 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc109)
      %47 = "acir.array.index"(%11, %c2_i32_19) {acir.id = "v205", acir.source_type = "bool", signed_index = false} : (!acir.mask<[3]>, i32) -> i1 loc(#loc110)
      %48 = scf.if %47 -> (i1) {
        scf.yield %46 : i1
      } else {
        %false_24 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_24 : i1
      }
      %49:2 = scf.if %48 -> (i1, i32) {
        %true_24 = arith.constant {acir.source_type = "bool"} true loc(#loc111)
        %c2_i32_25 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc112)
        scf.yield %true_24, %c2_i32_25 : i1, i32
      } else {
        scf.yield %37#0, %37#1 : i1, i32
      }
      %true_20 = arith.constant {acir.source_type = "bool"} true
      %true_21 = arith.constant {acir.source_type = "bool"} true
      %50 = acir.aggregate(%49#0, %49#1) {acir.id = "v219", acir.source_type = "tuple<bool,u32>"} : (i1, i32) -> tuple<i1, i32> loc(#loc113)
      %true_22 = arith.constant {acir.source_type = "bool"} true
      %false_23 = arith.constant {acir.source_type = "bool"} false
      %51 = acir.unary %false_23 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %51 {acir.source_type = "void"}
      acir.return %50 : tuple<i1, i32> {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.9.93.0.call.14.16 : (i32) -> () {acir.function = {contract = "Module:top.age", definition = "release", instance = "top.age", interface = "Module:top.age:release", kind = "rule", name = "top_body.0.call.9.93.0.call.14.16", output_type = "void", owner = "top.age", params = [{id = "v1", name = "position", targets = [], type = "u32"}], public = true, result = "void", rule = "release"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.age::@top.age.used {acir.source_type = "cell<array<bool;3>>"} : !acir.cell<!acir.array<i1, [3]>>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "array<bool;3>", role = "observe"} : !acir.cell<!acir.array<i1, [3]>> -> !acir.array<i1, [3]> loc(#loc114)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc115)
      %2 = acir.update %1, %false[%arg0] {acir.id = "v8", acir.source_type = "array<bool;3>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [3]>, i1, i32) -> !acir.array<i1, [3]> loc(#loc116)
      %3 = acir.ref @top.age::@top.age.used {acir.source_type = "cell<array<bool;3>>"} : !acir.cell<!acir.array<i1, [3]>>
      acir.cell.write %3, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [3]>>, !acir.array<i1, [3]> loc(#loc117)
      acir.return {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":18:40)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":44:40)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":44:39)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":45:43)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":45:42)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":45:41)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":85:43)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":86:41)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":87:42)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":88:40)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":89:43)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":93:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":94:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":100:21)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":98:27)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":101:13)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":102:13)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:22)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:31)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:13)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:16)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:9)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:19)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:29)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":108:9)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:17)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:25)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:9)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":115:12)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":115:21)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:22)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:25)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:29)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":116:13)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":117:14)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":117:23)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:22)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:25)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:29)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":118:13)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":119:14)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":119:23)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:22)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:25)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:29)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":120:13)
#loc47 = loc("-":121:35)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":121:29)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":121:28)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":122:12)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":122:21)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:27)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:55)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:71)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:19)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:13)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":124:12)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":124:21)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":125:19)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":125:13)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:12)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:21)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:27)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:35)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:40)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":127:13)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":128:9)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:24)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:20)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:23)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:17)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":22:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":26:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:20)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:36)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":37:17)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:17)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:9)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":30:17)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":31:12)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":32:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":32:27)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":33:13)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":49:17)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":50:18)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:41)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:35)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:20)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:13)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":53:35)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":53:30)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":53:13)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":54:27)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":54:9)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":55:9)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":56:9)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":66:16)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":66:47)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":67:18)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":68:17)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":69:20)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":71:22)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:25)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:20)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:39)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:32)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:42)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":74:30)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":75:21)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":75:16)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":76:25)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":77:28)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":78:9)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":60:17)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":61:27)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":61:9)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":62:9)

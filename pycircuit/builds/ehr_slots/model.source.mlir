module attributes {acir.config = {release_enabled = true}, acir.model = {config = [{name = "release_enabled", type = "bool"}], constants = {}, format_version = 9 : i64, top = "EHRSlots", type_order = [], types = {}}} {
  func.func @ac_init_top.slot.occupied(%arg0: i1) -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.slot.occupied", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "bool"}} {
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc1)
    return %true : i1
  }
  func.func @ac_init_top.slot.payload(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.slot.payload", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc2)
    return %c9_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_ehr_slots_1e0ac8be.EHRSlots", name = "top", parent, rules = ["allocate", "release"]}} {
    acir.contract {relations = [], rules = ["allocate", "release"]}
    func.func private @ac_init_top.slot.occupied(i1) -> i1
    func.func private @ac_init_top.slot.payload(i1) -> i32
    acir.rule @top_body.0.call.0.2 : () -> () {acir.function = {contract = "Module:top", definition = "release", instance = "top", interface = "Module:top:release", kind = "rule", name = "top_body.0.call.0.2", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "release"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      acir.rule.call @top.slot::@top_body.0.call.0.2.0.call.0.3() {acir.id = "v4", acir.source_type = "void", contract = "Module:top.slot", instance = "top.slot", interface = "Module:top.slot:release", role = "rule", rule = "release"} : () -> () loc(#loc3)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.2.6 : () -> () {acir.function = {contract = "Module:top", definition = "allocate", instance = "top", interface = "Module:top:allocate", kind = "rule", name = "top_body.0.call.2.6", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "allocate"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc4)
      acir.rule.call @top.slot::@top_body.0.call.2.6.0.call.2.4(%c7_i32) {acir.id = "v5", acir.source_type = "void", contract = "Module:top.slot", instance = "top.slot", interface = "Module:top.slot:allocate", role = "rule", rule = "allocate"} : (i32) -> () loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.config "release_enabled" {acir.id = "v2", acir.source_type = "bool"} : i1 loc(#loc6)
      scf.if %0 {
        acir.rule.call @top::@top_body.0.call.0.2() {acir.id = "v3", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:release", role = "rule", rule = "release"} : () -> () loc(#loc7)
      } else {
      }
      acir.rule.call @top::@top_body.0.call.2.6() {acir.id = "v7", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : () -> () loc(#loc8)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.slot attributes {acir.instance = {definition = "src_ehr_slots_1e0ac8be.Slot", name = "top.slot", parent = "top", rules = ["allocate", "available", "free", "release"]}} {
    acir.contract {relations = [{after = "available", before = "release", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 11 : i64}, relation = "SB", source = "user"}, {after = "allocate", before = "available", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 15 : i64}, relation = "SB", source = "user"}], rules = ["allocate", "available", "free", "release"]}
    acir.ehr @top.slot.occupied : i1 {initial_fn = "ac_init_top.slot.occupied", name = "top.slot.occupied", type = "bool"}
    acir.cell @top.slot.payload : i32 {initial_fn = "ac_init_top.slot.payload", name = "top.slot.payload", type = "u32"}
    func.func private @ac_init_top.slot.occupied(i1) -> i1
    func.func private @ac_init_top.slot.payload(i1) -> i32
    acir.rule @top_body.0.call.0.2.0.call.0.3 : () -> () {acir.function = {contract = "Module:top.slot", definition = "release", instance = "top.slot", interface = "Module:top.slot:release", kind = "rule", name = "top_body.0.call.0.2.0.call.0.3", output_type = "void", owner = "top.slot", params = [], public = true, result = "void", rule = "release"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc9)
      %0 = acir.ref @top.slot::@top.slot.occupied {acir.source_type = "ehr<bool>"} : !acir.ehr<i1>
      acir.ehr.write %0, %false {acir.id = "v6", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc10)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.2.6.0.call.2.4.0.call.2.1.0.call.2.6 : () -> i1 {acir.function = {contract = "Module:top.slot", definition = "available", instance = "top.slot", interface = "Module:top.slot:available", kind = "rule", name = "top_body.0.call.2.6.0.call.2.4.0.call.2.1.0.call.2.6", output_type = "bool", owner = "top.slot", params = [], public = true, result = "bool", rule = "available"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.slot::@top.slot.occupied {acir.source_type = "ehr<bool>"} : !acir.ehr<i1>
      %2 = acir.ehr.read %1 {acir.id = "v7", acir.source_type = "bool", role = "observe"} : !acir.ehr<i1> -> i1 loc(#loc11)
      %3 = acir.unary %2 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc12)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %4 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %4 {acir.source_type = "void"}
      acir.return %3 : i1 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.2.6.0.call.2.4.0.call.2.1 : (i32) -> i1 {acir.function = {contract = "Module:top.slot", definition = "free", instance = "top.slot", interface = "Module:top.slot:free", kind = "rule", name = "top_body.0.call.2.6.0.call.2.4.0.call.2.1", output_type = "bool", owner = "top.slot", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.rule.call @top.slot::@top_body.0.call.2.6.0.call.2.4.0.call.2.1.0.call.2.6() {acir.id = "v7", acir.source_type = "bool", contract = "Module:top.slot", instance = "top.slot", interface = "Module:top.slot:available", role = "rule", rule = "available"} : () -> i1 loc(#loc13)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %2 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %2 {acir.source_type = "void"}
      acir.return %1 : i1 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.2.6.0.call.2.4 : (i32) -> () {acir.function = {contract = "Module:top.slot", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.slot", interface = "Module:top.slot:allocate", kind = "rule", name = "top_body.0.call.2.6.0.call.2.4", output_type = "void", owner = "top.slot", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.slot::@top_body.0.call.2.6.0.call.2.4.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.slot", instance = "top.slot", interface = "Module:top.slot:free", role = "rule", rule = "free"} : (i32) -> i1 loc(#loc5)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc14)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc15)
      %0 = acir.ref @top.slot::@top.slot.occupied {acir.source_type = "ehr<bool>"} : !acir.ehr<i1>
      acir.ehr.write %0, %true_0 {acir.id = "v8", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc16)
      %1 = acir.ref @top.slot::@top.slot.payload {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %1, %arg0 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc17)
      acir.return {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":8:41)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":9:43)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":36:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":40:23)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":40:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":31:18)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":44:13)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":45:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":13:20)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":13:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":17:20)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":17:16)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":21:16)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":23:20)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":25:20)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":25:9)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":26:9)

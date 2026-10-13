module attributes {acir.config = {release_enabled = true}, acir.model = {config = [{name = "release_enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "EHRSlots", type_order = [], types = {}}} {
  func.func @ac_init_top.slot.occupied(%arg0: i1) -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.slot.occupied", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "bool"}} {
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc1)
    return %true : i1
  }
  func.func @ac_init_top.slot.payload(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.slot.payload", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc2)
    return %c9_i32 : i32
  }
  func.func @top_body.0.call.2.5.0.call.2.3.0.call.2.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.slot", kind = "guard", name = "top_body.0.call.2.5.0.call.2.3.0.call.2.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.rule.call @top.slot::@top_body.0.call.2.5.0.call.2.3.0.call.2.1.0.call.2.1() {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.slot", instance = "top.slot", interface = "Module:top.slot:available", role = "rule", rule = "available"} : () -> i1 loc(#loc3)
    return %0 : i1 loc(#loc4)
  }
  acir.module @top attributes {acir.instance = {definition = "src_ehr_slots_1e0ac8be.EHRSlots", inputs = {}, members = {}, name = "top", parent, rules = ["allocate", "release"]}} {
    acir.contract {relations = [], rules = ["allocate", "release"]}
    func.func private @ac_init_top.slot.occupied(i1) -> i1
    func.func private @ac_init_top.slot.payload(i1) -> i32
    func.func private @top_body.0.call.2.5.0.call.2.3.0.call.2.1(i32) -> i1
    acir.rule @top_body.0.call.0.2 : () -> () {acir.function = {contract = "Module:top", definition = "release", instance = "top", interface = "Module:top:release", kind = "rule", name = "top_body.0.call.0.2", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "release"}} body {
      acir.rule.call @top.slot::@top_body.0.call.0.2.0.call.0.2() {acir.id = "v3", acir.source_type = "void", contract = "Module:top.slot", instance = "top.slot", interface = "Module:top.slot:release", role = "rule", rule = "release"} : () -> () loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.2.5 : () -> () {acir.function = {contract = "Module:top", definition = "allocate", instance = "top", interface = "Module:top:allocate", kind = "rule", name = "top_body.0.call.2.5", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "allocate"}} body {
      %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc6)
      acir.rule.call @top.slot::@top_body.0.call.2.5.0.call.2.3(%c7_i32) {acir.id = "v4", acir.source_type = "void", contract = "Module:top.slot", instance = "top.slot", interface = "Module:top.slot:allocate", role = "rule", rule = "allocate"} : (i32) -> () loc(#loc7)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.config "release_enabled" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc8)
      scf.if %0 {
        acir.rule.call @top::@top_body.0.call.0.2() {acir.id = "v3", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:release", role = "rule", rule = "release"} : () -> () loc(#loc9)
      }
      acir.rule.call @top::@top_body.0.call.2.5() {acir.id = "v6", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:allocate", role = "rule", rule = "allocate"} : () -> () loc(#loc10)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.slot attributes {acir.instance = {definition = "src_ehr_slots_1e0ac8be.Slot", inputs = {}, members = {}, name = "top.slot", parent = "top", rules = ["allocate", "available", "release"]}} {
    acir.contract {relations = [{after = "available", before = "release", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 11 : i64}, relation = "SB", source = "user"}, {after = "allocate", before = "available", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 15 : i64}, relation = "SB", source = "user"}], rules = ["allocate", "available", "release"]}
    acir.ehr @top.slot.occupied : i1 {initial_fn = "ac_init_top.slot.occupied", name = "top.slot.occupied", type = "bool"}
    acir.cell @top.slot.payload : i32 {initial_fn = "ac_init_top.slot.payload", name = "top.slot.payload", type = "u32"}
    func.func private @ac_init_top.slot.occupied(i1) -> i1
    func.func private @ac_init_top.slot.payload(i1) -> i32
    func.func private @top_body.0.call.2.5.0.call.2.3.0.call.2.1(i32) -> i1
    acir.rule @top_body.0.call.0.2.0.call.0.2 : () -> () {acir.function = {contract = "Module:top.slot", definition = "release", instance = "top.slot", interface = "Module:top.slot:release", kind = "rule", name = "top_body.0.call.0.2.0.call.0.2", output_type = "void", owner = "top.slot", params = [], public = true, result = "void", rule = "release"}} body {
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc11)
      %0 = acir.ref @top.slot::@top.slot.occupied {acir.source_type = "ehr<bool>"} : !acir.ehr<i1>
      acir.ehr.write %0, %false {acir.id = "v5", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc12)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.2.5.0.call.2.3.0.call.2.1.0.call.2.1 : () -> i1 {acir.function = {contract = "Module:top.slot", definition = "available", instance = "top.slot", interface = "Module:top.slot:available", kind = "rule", name = "top_body.0.call.2.5.0.call.2.3.0.call.2.1.0.call.2.1", output_type = "bool", owner = "top.slot", params = [], public = true, result = "bool", rule = "available"}} body {
      %0 = acir.ref @top.slot::@top.slot.occupied {acir.source_type = "ehr<bool>"} : !acir.ehr<i1>
      %1 = acir.ehr.read %0 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.ehr<i1> -> i1 loc(#loc13)
      %2 = acir.unary %1 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc14)
      acir.return %2 : i1 {acir.id = "v6", acir.source_type = "void"} loc(#loc15)
    }
    acir.rule @top_body.0.call.2.5.0.call.2.3 : (i32) -> () {acir.function = {contract = "Module:top.slot", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.slot", interface = "Module:top.slot:allocate", kind = "rule", name = "top_body.0.call.2.5.0.call.2.3", output_type = "void", owner = "top.slot", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top_body.0.call.2.5.0.call.2.3.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.slot", role = "guard"} : (i32) -> i1 loc(#loc7)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc16)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc17)
      %0 = acir.ref @top.slot::@top.slot.occupied {acir.source_type = "ehr<bool>"} : !acir.ehr<i1>
      acir.ehr.write %0, %true {acir.id = "v7", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc18)
      %1 = acir.ref @top.slot::@top.slot.payload {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %1, %arg0 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc19)
      acir.return {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":8:41)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":9:43)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":20:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":20:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":35:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":39:23)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":39:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":30:18)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":43:13)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":44:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":13:20)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":13:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":17:20)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":17:16)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":17:9)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":22:20)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":24:20)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":24:9)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":25:9)

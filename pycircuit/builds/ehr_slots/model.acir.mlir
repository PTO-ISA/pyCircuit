module attributes {acir.model = {config = [{name = "release_enabled", type = "bool"}], constants = {}, exports = {}, format_version = 7 : i64, top = "EHRSlots", type_order = [], types = {}}} {
  func.func @ac_init_slot.occupied(%arg0: i1) -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_slot.occupied", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "bool"}} {
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc1)
    return %true : i1
  }
  func.func @ac_init_slot.payload(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_slot.payload", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc2)
    return %c9_i32 : i32
  }
  acir.module @slot attributes {acir.instance = {contract = "Module:slot", definition = "src_ehr_slots_1e0ac8be.Slot", inputs = {}, interfaces = {allocate = "Module:slot:allocate", available = "Module:slot:available", release = "Module:slot:release"}, members = {occupied = {resources = ["slot.occupied"], type = "ehr<bool>", value}, payload = {resources = ["slot.payload"], type = "cell<u32>", value}}, methods = ["allocate", "available", "release"], name = "slot", outputs, parent = "top"}} {
    acir.interface @allocate {implementations = [@slot::@top.allocate.0.call.1.3, @slot::@ac_summary_slot.allocate.0.call.6.1], orders = [{before = @slot::@available, loc = {column = 21 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 21 : i64}, origin = "slot.allocate", relation = "SB", source = "user"}]}
    acir.interface @available {implementations = [@slot::@top.allocate.0.call.1.3.0.call.1.1.0.call.1.1, @slot::@ac_summary_slot.available.0.call.5.0, @slot::@ac_summary_slot.allocate.0.call.6.1.0.call.6.1.0.call.6.1], orders = [{before = @slot::@release, loc = {column = 21 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 14 : i64}, origin = "slot.available", relation = "SB", source = "user"}]}
    acir.interface @release {implementations = [@slot::@top.release.0.call.0.2, @slot::@ac_summary_slot.release.0.call.4.0], orders = []}
    acir.ehr @slot.occupied : i1 {initial_fn = "ac_init_slot.occupied", name = "slot.occupied", type = "bool"}
    acir.cell @slot.payload : i32 {initial_fn = "ac_init_slot.payload", name = "slot.payload", type = "u32"}
    func.func private @ac_init_slot.occupied(i1) -> i1
    func.func private @ac_init_slot.payload(i1) -> i32
    acir.method @top.release.0.call.0.2 : () -> () {acir.function = {contract = "Module:slot", definition = "release", instance = "slot", interface = "Module:slot:release", kind = "method", method = "release", name = "top.release.0.call.0.2", params = [], result = "void"}, interface_ref = @slot::@release} body {
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc3)
      %0 = acir.ref @slot::@slot.occupied {acir.id = "v3", acir.source_type = "ehr<bool>"} : !acir.ehr<i1> loc(#loc4)
      acir.ehr.write %0, %false {acir.id = "v5", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.method @top.allocate.0.call.1.3.0.call.1.1.0.call.1.1 : () -> i1 {acir.function = {contract = "Module:slot", definition = "available", instance = "slot", interface = "Module:slot:available", kind = "method", method = "available", name = "top.allocate.0.call.1.3.0.call.1.1.0.call.1.1", params = [], result = "bool"}, interface_ref = @slot::@available} body {
      %0 = acir.ref @slot::@slot.occupied {acir.id = "v3", acir.source_type = "ehr<bool>"} : !acir.ehr<i1> loc(#loc4)
      %1 = acir.ehr.read %0 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.ehr<i1> -> i1 loc(#loc6)
      %2 = acir.unary %1 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc7)
      acir.return %2 : i1 {acir.id = "v6", acir.source_type = "void"} loc(#loc8)
    }
    func.func @top.allocate.0.call.1.3.0.call.1.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "free", instance = "slot", kind = "guard", name = "top.allocate.0.call.1.3.0.call.1.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.method.call @slot::@top.allocate.0.call.1.3.0.call.1.1.0.call.1.1() {acir.id = "v2", acir.source_type = "bool", contract = "Module:slot", instance = "slot", interface = "Module:slot:available", method = "available", role = "method"} : () -> i1 loc(#loc9)
      return %0 : i1 loc(#loc10)
    }
    acir.method @top.allocate.0.call.1.3 : (i32) -> () {acir.function = {contract = "Module:slot", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "slot", interface = "Module:slot:allocate", kind = "method", method = "allocate", name = "top.allocate.0.call.1.3", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @slot::@allocate} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.allocate.0.call.1.3.0.call.1.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "slot", role = "guard"} : (i32) -> i1 loc(#loc11)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc12)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc13)
      %0 = acir.ref @slot::@slot.occupied {acir.id = "v5", acir.source_type = "ehr<bool>"} : !acir.ehr<i1> loc(#loc4)
      acir.ehr.write %0, %true {acir.id = "v7", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc14)
      %1 = acir.ref @slot::@slot.payload {acir.id = "v8", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc15)
      acir.cell.write %1, %arg0 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc16)
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_slot.release.0.call.4.0 : () -> () {acir.function = {contract = "Module:slot", definition = "release", instance = "slot", interface = "Module:slot:release", kind = "method", method = "release", name = "ac_summary_slot.release.0.call.4.0", params = [], result = "void", summary = true}, interface_ref = @slot::@release} body {
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc3)
      %0 = acir.ref @slot::@slot.occupied {acir.id = "v3", acir.source_type = "ehr<bool>"} : !acir.ehr<i1> loc(#loc4)
      acir.ehr.write %0, %false {acir.id = "v5", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_slot.available.0.call.5.0 : () -> i1 {acir.function = {contract = "Module:slot", definition = "available", instance = "slot", interface = "Module:slot:available", kind = "method", method = "available", name = "ac_summary_slot.available.0.call.5.0", params = [], result = "bool", summary = true}, interface_ref = @slot::@available} body {
      %0 = acir.ref @slot::@slot.occupied {acir.id = "v3", acir.source_type = "ehr<bool>"} : !acir.ehr<i1> loc(#loc4)
      %1 = acir.ehr.read %0 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.ehr<i1> -> i1 loc(#loc6)
      %2 = acir.unary %1 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc7)
      acir.return %2 : i1 {acir.id = "v6", acir.source_type = "void"} loc(#loc8)
    }
    acir.method @ac_summary_slot.allocate.0.call.6.1.0.call.6.1.0.call.6.1 : () -> i1 {acir.function = {contract = "Module:slot", definition = "available", instance = "slot", interface = "Module:slot:available", kind = "method", method = "available", name = "ac_summary_slot.allocate.0.call.6.1.0.call.6.1.0.call.6.1", params = [], result = "bool"}, interface_ref = @slot::@available} body {
      %0 = acir.ref @slot::@slot.occupied {acir.id = "v3", acir.source_type = "ehr<bool>"} : !acir.ehr<i1> loc(#loc4)
      %1 = acir.ehr.read %0 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.ehr<i1> -> i1 loc(#loc6)
      %2 = acir.unary %1 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc7)
      acir.return %2 : i1 {acir.id = "v6", acir.source_type = "void"} loc(#loc8)
    }
    func.func @ac_summary_slot.allocate.0.call.6.1.0.call.6.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "free", instance = "slot", kind = "guard", name = "ac_summary_slot.allocate.0.call.6.1.0.call.6.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.method.call @slot::@ac_summary_slot.allocate.0.call.6.1.0.call.6.1.0.call.6.1() {acir.id = "v2", acir.source_type = "bool", contract = "Module:slot", instance = "slot", interface = "Module:slot:available", method = "available", role = "method"} : () -> i1 loc(#loc9)
      return %0 : i1 loc(#loc10)
    }
    acir.method @ac_summary_slot.allocate.0.call.6.1 : (i32) -> () {acir.function = {contract = "Module:slot", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "slot", interface = "Module:slot:allocate", kind = "method", method = "allocate", name = "ac_summary_slot.allocate.0.call.6.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @slot::@allocate} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_slot.allocate.0.call.6.1.0.call.6.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "slot", role = "guard"} : (i32) -> i1 loc(#loc17)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc12)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc13)
      %0 = acir.ref @slot::@slot.occupied {acir.id = "v5", acir.source_type = "ehr<bool>"} : !acir.ehr<i1> loc(#loc4)
      acir.ehr.write %0, %true {acir.id = "v7", acir.source_type = "void"} : !acir.ehr<i1>, i1 loc(#loc14)
      %1 = acir.ref @slot::@slot.payload {acir.id = "v8", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc15)
      acir.cell.write %1, %arg0 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc16)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_ehr_slots_1e0ac8be.EHRSlots", inputs = {release_enabled = {resources = [], type = "bool", value}}, interfaces = {}, members = {release_enabled = {resources = [], type = "bool", value}, slot = {instance = "slot"}}, methods = [], name = "top", outputs, parent}} {
    acir.instance @slot = @slot
    func.func private @ac_init_slot.occupied(i1) -> i1
    func.func private @ac_init_slot.payload(i1) -> i32
    acir.rule @top.release : () -> () {acir.function = {kind = "rule", name = "top.release", owner = "top", params = [], result = "void", rule = "release"}, output_types = []} body {
      acir.method.call @slot::@top.release.0.call.0.2() {acir.id = "v3", acir.source_type = "void", contract = "Module:slot", instance = "slot", interface = "Module:slot:release", method = "release", role = "method"} : () -> () loc(#loc18)
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.rule @top.allocate : () -> () {acir.function = {kind = "rule", name = "top.allocate", owner = "top", params = [], result = "void", rule = "allocate"}, output_types = []} body {
      %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc19)
      acir.method.call @slot::@top.allocate.0.call.1.3(%c7_i32) {acir.id = "v4", acir.source_type = "void", contract = "Module:slot", instance = "slot", interface = "Module:slot:allocate", method = "allocate", role = "method"} : (i32) -> () loc(#loc11)
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
      %0 = acir.config "release_enabled" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc20)
      scf.if %0 {
        acir.rule.call @top::@top.release() {acir.id = "v3", acir.source_type = "void", output_bindings = []} : () -> () loc(#loc21)
      }
      acir.rule.call @top::@top.allocate() {acir.id = "v6", acir.source_type = "void", output_bindings = []} : () -> () loc(#loc22)
      acir.work.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":7:37)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":8:39)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":12:26)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":7:16)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":12:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":16:20)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":16:16)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":16:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":19:16)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":19:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":37:9)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":21:41)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":23:26)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":23:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":8:15)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":24:9)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":22:5)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":33:9)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":37:23)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":28:14)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":40:9)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ehr_slots.py":41:5)


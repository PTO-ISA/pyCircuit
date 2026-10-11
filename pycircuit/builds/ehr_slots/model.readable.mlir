module attributes {acir.ehr = [{aspect = "value", bindings = [{method = "$direct", port = 0 : i64}, {method = "Module:slot:allocate", port = 2 : i64}, {method = "Module:slot:available", port = 1 : i64}, {method = "Module:slot:release", port = 0 : i64}], commit = "transaction", fallback = "old", port_count = 3 : i64, resource = "slot.occupied", same_port = "invisible", semantics = "last_effective_prior_port"}], acir.model = {config = [{name = "release_enabled", type = "bool"}], constants = {}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:slot", methods = ["allocate", "available", "release"], relations = [{after = "available", before = "release", loc = {column = 21 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 14 : i64}, origin = "slot.available", relation = "SB", source = "user"}, {after = "allocate", before = "available", loc = {column = 21 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 21 : i64}, origin = "slot.allocate", relation = "SB", source = "user"}]}, {id = "Module:top", methods = [], relations = []}], exports = {}, format_version = 6 : i64, instances = [{contract = "Module:slot", definition = "src_ehr_slots_1e0ac8be.Slot", inputs = {}, interfaces = {allocate = "Module:slot:allocate", available = "Module:slot:available", release = "Module:slot:release"}, members = {occupied = {resources = ["slot.occupied"], type = "cell<bool>", value}, payload = {resources = ["slot.payload"], type = "cell<u32>", value}}, methods = ["allocate", "available", "release"], name = "slot", outputs, parent = "top"}, {contract = "Module:top", definition = "src_ehr_slots_1e0ac8be.EHRSlots", inputs = {release_enabled = {resources = [], type = "bool", value}}, interfaces = {}, members = {release_enabled = {resources = [], type = "bool", value}, slot = {instance = "slot"}}, methods = [], name = "top", outputs, parent}], modules = [{name = "top", rules = [{function = "top.release", name = "release"}, {function = "top.allocate", name = "allocate"}], work = "top_Work"}], top = "EHRSlots", type_order = [], types = {}}} {
  acir.resource "slot.occupied" "cell" "bool" {contract = "EHR", initial_fn = "ac_init_slot.occupied", loc = {column = 15 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 7 : i64}, visibility = "ehr"}
  acir.resource "slot.payload" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_slot.payload", loc = {column = 14 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_slots.py", line = 8 : i64}, visibility = "old"}
  func.func @ac_init_slot.occupied(%arg0: i1) -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_slot.occupied", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "bool"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    return %true : i1
  }
  func.func @ac_init_slot.payload(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_slot.payload", params = [{id = "cfg0", name = "release_enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32
    return %c9_i32 : i32
  }
  func.func @top.release.0.call.0.0() attributes {acir.function = {contract = "Module:slot", definition = "release", instance = "slot", interface = "Module:slot:release", kind = "method", method = "release", name = "top.release.0.call.0.0", params = [], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "slot.occupied" {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    acir.write %0, %false {acir.id = "v3", acir.source_type = "void", port = 0 : i64, ports = [{aspect = "value", port = 0 : i64, resource = "slot.occupied"}], view = "ehr"} : !acir.cell<i1>, i1
    return
  }
  func.func @top.allocate.0.call.1.1.0.call.1.1.0.call.1.1() -> i1 attributes {acir.function = {contract = "Module:slot", definition = "available", instance = "slot", interface = "Module:slot:available", kind = "guard", method = "available", name = "top.allocate.0.call.1.1.0.call.1.1.0.call.1.1", params = [], result = "bool"}} {
    %0 = acir.get "slot.occupied" {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "bool", port = 1 : i64, ports = [{aspect = "value", port = 1 : i64, resource = "slot.occupied"}], role = "observe", view = "ehr"} : !acir.cell<i1> -> i1
    %2 = acir.unary %1 "not" {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
    return %2 : i1
  }
  func.func @top.allocate.0.call.1.1.0.call.1.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "free", instance = "slot", kind = "guard", name = "top.allocate.0.call.1.1.0.call.1.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "bool"}} {
    %0 = call @top.allocate.0.call.1.1.0.call.1.1.0.call.1.1() {acir.id = "v2", acir.source_type = "bool", contract = "Module:slot", instance = "slot", interface = "Module:slot:available", method = "available", role = "guard"} : () -> i1
    return %0 : i1
  }
  func.func @top.allocate.0.call.1.1(%arg0: i32) attributes {acir.function = {contract = "Module:slot", definition = "allocate", instance = "slot", interface = "Module:slot:allocate", kind = "method", method = "allocate", name = "top.allocate.0.call.1.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @top.allocate.0.call.1.1.0.call.1.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "slot", role = "guard"} : (i32) -> i1
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "slot.occupied" {acir.id = "v4", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    acir.write %1, %true {acir.id = "v6", acir.source_type = "void", port = 2 : i64, ports = [{aspect = "value", port = 2 : i64, resource = "slot.occupied"}], view = "ehr"} : !acir.cell<i1>, i1
    %2 = acir.get "slot.payload" {acir.id = "v7", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %2, %arg0 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @ac_summary_slot.release.0.call.4.0() attributes {acir.function = {contract = "Module:slot", definition = "release", instance = "slot", interface = "Module:slot:release", kind = "method", method = "release", name = "ac_summary_slot.release.0.call.4.0", params = [], result = "void", summary = true}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "slot.occupied" {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    acir.write %0, %false {acir.id = "v3", acir.source_type = "void", port = 0 : i64, ports = [{aspect = "value", port = 0 : i64, resource = "slot.occupied"}], view = "ehr"} : !acir.cell<i1>, i1
    return
  }
  func.func @ac_summary_slot.available.0.call.5.0() -> i1 attributes {acir.function = {contract = "Module:slot", definition = "available", instance = "slot", interface = "Module:slot:available", kind = "method", method = "available", name = "ac_summary_slot.available.0.call.5.0", params = [], result = "bool", summary = true}} {
    %0 = acir.get "slot.occupied" {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "bool", port = 1 : i64, ports = [{aspect = "value", port = 1 : i64, resource = "slot.occupied"}], role = "observe", view = "ehr"} : !acir.cell<i1> -> i1
    %2 = acir.unary %1 "not" {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
    return %2 : i1
  }
  func.func @ac_summary_slot.allocate.0.call.6.1.0.call.6.1.0.call.6.1() -> i1 attributes {acir.function = {contract = "Module:slot", definition = "available", instance = "slot", interface = "Module:slot:available", kind = "guard", method = "available", name = "ac_summary_slot.allocate.0.call.6.1.0.call.6.1.0.call.6.1", params = [], result = "bool"}} {
    %0 = acir.get "slot.occupied" {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "bool", port = 1 : i64, ports = [{aspect = "value", port = 1 : i64, resource = "slot.occupied"}], role = "observe", view = "ehr"} : !acir.cell<i1> -> i1
    %2 = acir.unary %1 "not" {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
    return %2 : i1
  }
  func.func @ac_summary_slot.allocate.0.call.6.1.0.call.6.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "free", instance = "slot", kind = "guard", name = "ac_summary_slot.allocate.0.call.6.1.0.call.6.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "bool"}} {
    %0 = call @ac_summary_slot.allocate.0.call.6.1.0.call.6.1.0.call.6.1() {acir.id = "v2", acir.source_type = "bool", contract = "Module:slot", instance = "slot", interface = "Module:slot:available", method = "available", role = "guard"} : () -> i1
    return %0 : i1
  }
  func.func @ac_summary_slot.allocate.0.call.6.1(%arg0: i32) attributes {acir.function = {contract = "Module:slot", definition = "allocate", instance = "slot", interface = "Module:slot:allocate", kind = "method", method = "allocate", name = "ac_summary_slot.allocate.0.call.6.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_slot.allocate.0.call.6.1.0.call.6.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "slot", role = "guard"} : (i32) -> i1
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "slot.occupied" {acir.id = "v4", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    acir.write %1, %true {acir.id = "v6", acir.source_type = "void", port = 2 : i64, ports = [{aspect = "value", port = 2 : i64, resource = "slot.occupied"}], view = "ehr"} : !acir.cell<i1>, i1
    %2 = acir.get "slot.payload" {acir.id = "v7", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %2, %arg0 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @top.release() attributes {acir.function = {kind = "rule", name = "top.release", outputs = [], owner = "top", params = [], result = "void", rule = "release"}} {
    call @top.release.0.call.0.0() {acir.id = "v1", acir.source_type = "void", contract = "Module:slot", instance = "slot", interface = "Module:slot:release", method = "release", role = "method"} : () -> ()
    return
  }
  func.func @top.allocate() attributes {acir.function = {kind = "rule", name = "top.allocate", outputs = [], owner = "top", params = [], result = "void", rule = "allocate"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    call @top.allocate.0.call.1.1(%c7_i32) {acir.id = "v2", acir.source_type = "void", contract = "Module:slot", instance = "slot", interface = "Module:slot:allocate", method = "allocate", role = "method"} : (i32) -> ()
    return
  }
  func.func @top_Work() attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
    %0 = acir.config "release_enabled" {acir.id = "v1", acir.source_type = "bool"} : i1
    cf.cond_br %0, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "release"() {acir.id = "v3", acir.source_type = "void", callee = "top.release"} : () -> ()
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    acir.invoke "allocate"() {acir.id = "v6", acir.source_type = "void", callee = "top.allocate"} : () -> ()
    return
  }
}


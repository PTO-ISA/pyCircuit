module attributes {acir.ehr = [{aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "left_out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "right_out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "source", same_port = "invisible", semantics = "last_effective_prior_port"}], acir.model = {config = [], constants = {}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:observer", methods = ["peek"], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {left_out = "left_out", right_out = "right_out", source = "source"}, format_version = 6 : i64, instances = [{contract = "Module:observer", definition = "src_dual_channel_3c0614f6.Observer", inputs = {}, interfaces = {peek = "Module:observer:peek"}, members = {}, methods = ["peek"], name = "observer", outputs, parent = "top"}, {contract = "Module:top", definition = "src_dual_channel_3c0614f6.DualChannel", inputs = {}, interfaces = {}, members = {left_out = {resources = ["left_out"], type = "queue<u32>", value}, observer = {instance = "observer"}, right_out = {resources = ["right_out"], type = "queue<u32>", value}, source = {resources = ["source"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["left_out"], type = "queue<u32>", value}, {resources = ["right_out"], type = "queue<u32>", value}], parent}], modules = [{name = "top", rules = [{function = "top.left", name = "left"}, {function = "top.right", name = "right"}], work = "top_Work"}], top = "DualChannel", type_order = [], types = {}}} {
  acir.resource "source" "queue" "u32" {capacity_fn = "ac_init_source_cap", contract = "Queue", initial_fn = "ac_init_source", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/dual_channel.py", line = 18 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "left_out" "queue" "u32" {capacity_fn = "ac_init_left_out_cap", contract = "Queue", initial_fn = "ac_init_left_out", loc = {column = 15 : i64, file = "/home/lc/tmp/pycircuit/examples/dual_channel.py", line = 19 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "right_out" "queue" "u32" {capacity_fn = "ac_init_right_out_cap", contract = "Queue", initial_fn = "ac_init_right_out", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/dual_channel.py", line = 20 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  func.func @ac_init_source_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_source_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_source() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_source", params = [], result = "vector<u32>"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc1)
    %0 = acir.aggregate(%c7_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_left_out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_left_out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_left_out() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_left_out", params = [], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_right_out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_right_out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_right_out() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_right_out", params = [], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @top_Work.0.call.0.1(%arg0: !acir.queue<i32>) -> i32 attributes {acir.function = {contract = "Module:observer", definition = "peek", instance = "observer", interface = "Module:observer:peek", kind = "method", method = "peek", name = "top_Work.0.call.0.1", params = [{consume = false, id = "v1", name = "q", targets = ["source"], type = "queue<u32>"}], result = "u32"}} {
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.queue<i32> -> i32 loc(#loc2)
    return %0 : i32 loc(#loc3)
  }
  func.func @top.left(%arg0: i32) attributes {acir.function = {kind = "rule", name = "top.left", outputs = [{kind = "queue", name = "left_out", targets = ["left_out"], type = "u32"}], owner = "top", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", rule = "left"}} {
    %0 = acir.get "left_out" {acir.id = "v2", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc4)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "left_out"}], view = "ehr"} : !acir.queue<i32> -> i1 loc(#loc5)
    %2 = acir.unary %1 "not" {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1 loc(#loc5)
    acir.guard %2 "Queue.push capacity" {acir.id = "v5", acir.source_type = "void", implicit = true} loc(#loc5)
    acir.push %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc5)
    return loc(#loc5)
  }
  func.func @top.right(%arg0: i32) attributes {acir.function = {kind = "rule", name = "top.right", outputs = [{kind = "queue", name = "right_out", targets = ["right_out"], type = "u32"}], owner = "top", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", rule = "right"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc6)
    %0 = arith.addi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc7)
    %1 = acir.get "right_out" {acir.id = "v4", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc8)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "right_out"}], view = "ehr"} : !acir.queue<i32> -> i1 loc(#loc9)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc9)
    acir.guard %3 "Queue.push capacity" {acir.id = "v7", acir.source_type = "void", implicit = true} loc(#loc9)
    acir.push %1, %0 {acir.id = "v8", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc9)
    return loc(#loc9)
  }
  func.func @top_Work() attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc10)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc11)
    %0 = acir.get "source" {acir.id = "v1", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc12)
    %1 = call @top_Work.0.call.0.1(%0) {acir.id = "v2", acir.source_type = "u32", contract = "Module:observer", instance = "observer", interface = "Module:observer:peek", method = "peek", role = "method"} : (!acir.queue<i32>) -> i32 loc(#loc13)
    %2 = arith.addi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc13)
    %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc14)
    cf.cond_br %3, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "left"(%2) {acir.id = "v8", acir.source_type = "void", callee = "top.left"} : (i32) -> () loc(#loc15)
    acir.invoke "right"(%2) {acir.id = "v10", acir.source_type = "void", callee = "top.right"} : (i32) -> () loc(#loc16)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":18:39)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":12:16)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":12:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":19:16)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":24:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":28:24)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":28:16)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":20:17)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":28:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":31:18)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":30:38)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":18:14)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":30:14)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":31:8)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":32:20)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":33:21)


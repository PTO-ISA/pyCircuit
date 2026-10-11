module attributes {acir.ehr = [{aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "left", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "right", same_port = "invisible", semantics = "last_effective_prior_port"}], acir.model = {config = [], constants = {}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {left = "left", right = "right"}, format_version = 6 : i64, instances = [{contract = "Module:top", definition = "src_dependency_cycles_63d6eeaf.QueueRing", inputs = {}, interfaces = {}, members = {left = {resources = ["left"], type = "queue<u32>", value}, right = {resources = ["right"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["left"], type = "queue<u32>", value}, {resources = ["right"], type = "queue<u32>", value}], parent}], modules = [{name = "top", rules = [{function = "top.forward", name = "forward"}, {function = "top.backward", name = "backward"}], work = "top_Work"}], top = "QueueRing", type_order = [], types = {}}} {
  acir.resource "left" "queue" "u32" {capacity_fn = "ac_init_left_cap", contract = "Queue", initial_fn = "ac_init_left", loc = {column = 11 : i64, file = "/home/lc/tmp/pycircuit/examples/dependency_cycles.py", line = 45 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "right" "queue" "u32" {capacity_fn = "ac_init_right_cap", contract = "Queue", initial_fn = "ac_init_right", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/dependency_cycles.py", line = 46 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  func.func @ac_init_left_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_left_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_left() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_left", params = [], result = "vector<u32>"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    %0 = acir.aggregate(%c1_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_right_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_right_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_right() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_right", params = [], result = "vector<u32>"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    %0 = acir.aggregate(%c2_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @top.forward(%arg0: !acir.queue<i32>) attributes {acir.function = {kind = "rule", name = "top.forward", outputs = [{kind = "queue", name = "right", targets = ["right"], type = "u32"}], owner = "top", params = [{consume = true, id = "v1", name = "source", targets = ["left"], type = "queue<u32>"}], result = "void", rule = "forward"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc3)
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc3)
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<i32> loc(#loc3)
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "left"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc3)
    %1 = acir.get "right" {acir.id = "v6", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc4)
    %2 = acir.read %1 {acir.id = "v7", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "right"}], view = "ehr"} : !acir.queue<i32> -> i1 loc(#loc5)
    %3 = acir.unary %2 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc5)
    acir.guard %3 "Queue.push capacity" {acir.id = "v9", acir.source_type = "void", implicit = true} loc(#loc5)
    acir.push %1, %0 {acir.id = "v10", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc5)
    return loc(#loc5)
  }
  func.func @top.backward(%arg0: !acir.queue<i32>) attributes {acir.function = {kind = "rule", name = "top.backward", outputs = [{kind = "queue", name = "left", targets = ["left"], type = "u32"}], owner = "top", params = [{consume = true, id = "v1", name = "source", targets = ["right"], type = "queue<u32>"}], result = "void", rule = "backward"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc6)
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc6)
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<i32> loc(#loc6)
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "right"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc6)
    %1 = acir.get "left" {acir.id = "v6", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc7)
    %2 = acir.read %1 {acir.id = "v7", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "left"}], view = "ehr"} : !acir.queue<i32> -> i1 loc(#loc8)
    %3 = acir.unary %2 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc8)
    acir.guard %3 "Queue.push capacity" {acir.id = "v9", acir.source_type = "void", implicit = true} loc(#loc8)
    acir.push %1, %0 {acir.id = "v10", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc8)
    return loc(#loc8)
  }
  func.func @top_Work() attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
    %0 = acir.get "left" {acir.id = "v1", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc7)
    acir.invoke "forward"(%0) {acir.id = "v2", acir.source_type = "void", callee = "top.forward"} : (!acir.queue<i32>) -> () loc(#loc9)
    %1 = acir.get "right" {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc4)
    acir.invoke "backward"(%1) {acir.id = "v4", acir.source_type = "void", callee = "top.backward"} : (!acir.queue<i32>) -> () loc(#loc10)
    return
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":45:37)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":46:38)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":50:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":46:13)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":50:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":54:16)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":45:12)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":54:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":56:13)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":57:12)


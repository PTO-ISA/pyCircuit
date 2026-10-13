module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "QueueRing", type_order = [], types = {}}} {
  func.func @ac_init_top.left_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.left_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.left() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.left", params = [], result = "vector<u32>"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    %0 = acir.aggregate(%c1_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.right_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.right_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.right() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.right", params = [], result = "vector<u32>"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    %0 = acir.aggregate(%c2_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  acir.module @top attributes {acir.instance = {definition = "src_dependency_cycles_63d6eeaf.QueueRing", inputs = {}, members = {}, name = "top", parent, rules = ["backward", "forward"]}} {
    acir.contract {relations = [], rules = ["backward", "forward"]}
    acir.queue @top.left : i32 {capacity_fn = "ac_init_top.left_cap", initial_fn = "ac_init_top.left", name = "top.left", type = "u32"}
    acir.queue @top.right : i32 {capacity_fn = "ac_init_top.right_cap", initial_fn = "ac_init_top.right", name = "top.right", type = "u32"}
    func.func private @ac_init_top.left_cap() -> i32
    func.func private @ac_init_top.left() -> !acir.vector<i32>
    func.func private @ac_init_top.right_cap() -> i32
    func.func private @ac_init_top.right() -> !acir.vector<i32>
    acir.rule @top_body.0.call.0.1 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top", definition = "forward", instance = "top", interface = "Module:top:forward", kind = "rule", name = "top_body.0.call.0.1", output_type = "queue<u32>", owner = "top", params = [{consume = false, id = "v1", name = "source", targets = ["top.left"], type = "queue<u32>"}], public = true, result = "u32", rule = "forward"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc3)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc4)
    }
    acir.rule @top_body.0.call.1.4 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top", definition = "backward", instance = "top", interface = "Module:top:backward", kind = "rule", name = "top_body.0.call.1.4", output_type = "queue<u32>", owner = "top", params = [{consume = false, id = "v1", name = "source", targets = ["top.right"], type = "queue<u32>"}], public = true, result = "u32", rule = "backward"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc5)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc6)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.ref @top::@top.left {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.rule.call @top::@top_body.0.call.0.1(%0) {acir.id = "v2", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:forward", output_binding = @top::@top.right, role = "rule", rule = "forward"} : (!acir.queue<i32>) -> i32 loc(#loc7)
      %2 = acir.ref @top::@top.right {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %3 = acir.rule.call @top::@top_body.0.call.1.4(%2) {acir.id = "v5", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:backward", output_binding = @top::@top.left, role = "rule", rule = "backward"} : (!acir.queue<i32>) -> i32 loc(#loc8)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":48:41)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":49:42)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":53:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":53:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":57:16)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":57:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":60:17)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":61:16)

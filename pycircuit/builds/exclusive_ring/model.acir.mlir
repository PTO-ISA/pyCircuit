module attributes {acir.config = {choose = true}, acir.model = {config = [{name = "choose", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "ExclusiveRing", type_order = [], types = {}}} {
  func.func @ac_init_top.left_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.left_cap", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.left(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.left", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    %0 = acir.aggregate(%c1_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.right_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.right_cap", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.right(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.right", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    %0 = acir.aggregate(%c2_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  acir.module @top attributes {acir.instance = {definition = "src_dependency_cycles_63d6eeaf.ExclusiveRing", inputs = {}, members = {}, name = "top", outputs, parent, rules = ["backward", "forward"]}} {
    acir.contract {relations = [], rules = ["backward", "forward"]}
    acir.queue @top.left : i32 {capacity_fn = "ac_init_top.left_cap", initial_fn = "ac_init_top.left", name = "top.left", type = "u32"}
    acir.queue @top.right : i32 {capacity_fn = "ac_init_top.right_cap", initial_fn = "ac_init_top.right", name = "top.right", type = "u32"}
    func.func private @ac_init_top.left_cap(i1) -> i32
    func.func private @ac_init_top.left(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right_cap(i1) -> i32
    func.func private @ac_init_top.right(i1) -> !acir.vector<i32>
    acir.rule @top_body.0.call.0.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top", definition = "forward", instance = "top", interface = "Module:top:forward", kind = "rule", name = "top_body.0.call.0.3", output_type = "queue<u32>", owner = "top", params = [{consume = false, id = "v1", name = "source", targets = ["top.left"], type = "queue<u32>"}], public = true, result = "u32", rule = "forward"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc3)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc4)
    }
    acir.rule @top_body.0.call.1.6 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top", definition = "backward", instance = "top", interface = "Module:top:backward", kind = "rule", name = "top_body.0.call.1.6", output_type = "queue<u32>", owner = "top", params = [{consume = false, id = "v1", name = "source", targets = ["top.right"], type = "queue<u32>"}], public = true, result = "u32", rule = "backward"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc5)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc6)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.config "choose" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc7)
      scf.if %0 {
        %1 = acir.ref @top::@top.left {acir.source_type = "queue<u32>"} : !acir.queue<i32>
        %2 = acir.rule.call @top::@top_body.0.call.0.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:forward", output_binding = @top::@top.right, role = "rule", rule = "forward"} : (!acir.queue<i32>) -> i32 loc(#loc8)
      } else {
        %1 = acir.ref @top::@top.right {acir.source_type = "queue<u32>"} : !acir.queue<i32>
        %2 = acir.rule.call @top::@top_body.0.call.1.6(%1) {acir.id = "v7", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:backward", output_binding = @top::@top.left, role = "rule", rule = "backward"} : (!acir.queue<i32>) -> i32 loc(#loc9)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":28:41)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":29:42)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":33:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":33:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":37:16)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":37:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":27:18)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":41:21)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":43:20)

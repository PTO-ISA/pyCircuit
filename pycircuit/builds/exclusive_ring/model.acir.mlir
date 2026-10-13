module attributes {acir.model = {config = [{name = "choose", type = "bool"}], constants = {}, exports = {left = "left", right = "right"}, format_version = 7 : i64, top = "ExclusiveRing", type_order = [], types = {}}} {
  func.func @ac_init_left_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_left_cap", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_left(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_left", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    %0 = acir.aggregate(%c1_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_right_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_right_cap", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_right(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_right", params = [{id = "cfg0", name = "choose", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    %0 = acir.aggregate(%c2_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_dependency_cycles_63d6eeaf.ExclusiveRing", inputs = {choose = {resources = [], type = "bool", value}}, interfaces = {}, members = {choose = {resources = [], type = "bool", value}, left = {resources = ["left"], type = "queue<u32>", value}, right = {resources = ["right"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["left"], type = "queue<u32>", value}, {resources = ["right"], type = "queue<u32>", value}], parent}} {
    acir.queue @left : i32 {capacity_fn = "ac_init_left_cap", initial_fn = "ac_init_left", name = "left", type = "u32"}
    acir.queue @right : i32 {capacity_fn = "ac_init_right_cap", initial_fn = "ac_init_right", name = "right", type = "u32"}
    func.func private @ac_init_left_cap(i1) -> i32
    func.func private @ac_init_left(i1) -> !acir.vector<i32>
    func.func private @ac_init_right_cap(i1) -> i32
    func.func private @ac_init_right(i1) -> !acir.vector<i32>
    acir.rule @top.forward : (!acir.queue<i32>) -> () {acir.function = {kind = "rule", name = "top.forward", owner = "top", params = [{consume = true, id = "v1", name = "source", targets = ["left"], type = "queue<u32>"}], result = "void", rule = "forward"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc3)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void", slots = [0]} loc(#loc4)
    }
    acir.rule @top.backward : (!acir.queue<i32>) -> () {acir.function = {kind = "rule", name = "top.backward", owner = "top", params = [{consume = true, id = "v1", name = "source", targets = ["right"], type = "queue<u32>"}], result = "void", rule = "backward"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc5)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void", slots = [0]} loc(#loc6)
    }
    acir.work attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
      %0 = acir.config "choose" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc7)
      %1 = acir.ref @top::@left {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc8)
      %2 = acir.ref @top::@right {acir.id = "v5", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc9)
      scf.if %0 {
        acir.rule.call @top::@top.forward(%1) {acir.id = "v4", acir.source_type = "void", output_bindings = [@top::@right]} : (!acir.queue<i32>) -> () loc(#loc10)
      } else {
        acir.rule.call @top::@top.backward(%2) {acir.id = "v6", acir.source_type = "void", output_bindings = [@top::@left]} : (!acir.queue<i32>) -> () loc(#loc11)
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":25:37)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":26:38)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":30:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":30:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":34:16)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":34:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":24:19)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":25:12)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":26:13)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":37:17)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":39:16)


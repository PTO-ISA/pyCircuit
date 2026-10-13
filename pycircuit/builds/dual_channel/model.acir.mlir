module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "DualChannel", type_order = [], types = {}}} {
  func.func @ac_init_top.source_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.source_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.source() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.source", params = [], result = "vector<u32>"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc1)
    %0 = acir.aggregate(%c7_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.left_out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.left_out() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out", params = [], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.right_out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.right_out() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out", params = [], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  acir.module @top attributes {acir.instance = {definition = "src_dual_channel_3c0614f6.DualChannel", inputs = {}, members = {}, name = "top", outputs, parent, rules = ["left", "right"]}} {
    acir.contract {relations = [], rules = ["left", "right"]}
    acir.queue @top.source : i32 {capacity_fn = "ac_init_top.source_cap", initial_fn = "ac_init_top.source", name = "top.source", type = "u32"}
    acir.queue @top.left_out : i32 {capacity_fn = "ac_init_top.left_out_cap", initial_fn = "ac_init_top.left_out", name = "top.left_out", type = "u32"}
    acir.queue @top.right_out : i32 {capacity_fn = "ac_init_top.right_out_cap", initial_fn = "ac_init_top.right_out", name = "top.right_out", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap() -> i32
    func.func private @ac_init_top.left_out() -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap() -> i32
    func.func private @ac_init_top.right_out() -> !acir.vector<i32>
    acir.rule @top_body.0.call.1.7 : (i32) -> i32 {acir.function = {contract = "Module:top", definition = "left", instance = "top", interface = "Module:top:left", kind = "rule", name = "top_body.0.call.1.7", output_type = "queue<u32>", owner = "top", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "u32", rule = "left"}} body {
    ^bb0(%arg0: i32):
      acir.return %arg0 : i32 {acir.id = "v4", acir.source_type = "void"} loc(#loc2)
    }
    acir.rule @top_body.0.call.2.9 : (i32) -> i32 {acir.function = {contract = "Module:top", definition = "right", instance = "top", interface = "Module:top:right", kind = "rule", name = "top_body.0.call.2.9", output_type = "queue<u32>", owner = "top", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "u32", rule = "right"}} body {
    ^bb0(%arg0: i32):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc3)
      %0 = arith.addi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc4)
      acir.return %0 : i32 {acir.id = "v6", acir.source_type = "void"} loc(#loc5)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc6)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc7)
      %0 = acir.ref @top::@top.source {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.rule.call @top.observer::@top_body.0.call.0.1(%0) {acir.id = "v2", acir.source_type = "u32", contract = "Module:top.observer", instance = "top.observer", interface = "Module:top.observer:peek", role = "rule", rule = "peek"} : (!acir.queue<i32>) -> i32 loc(#loc8)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc8)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc9)
      scf.if %3 {
        %4 = acir.rule.call @top::@top_body.0.call.1.7(%2) {acir.id = "v8", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:left", output_binding = @top::@top.left_out, role = "rule", rule = "left"} : (i32) -> i32 loc(#loc10)
        %5 = acir.rule.call @top::@top_body.0.call.2.9(%2) {acir.id = "v10", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:right", output_binding = @top::@top.right_out, role = "rule", rule = "right"} : (i32) -> i32 loc(#loc11)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.observer attributes {acir.instance = {definition = "src_dual_channel_3c0614f6.Observer", inputs = {}, members = {}, name = "top.observer", outputs, parent = "top", rules = ["peek"]}} {
    acir.contract {relations = [], rules = ["peek"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap() -> i32
    func.func private @ac_init_top.left_out() -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap() -> i32
    func.func private @ac_init_top.right_out() -> !acir.vector<i32>
    acir.rule @top_body.0.call.0.1 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.observer", definition = "peek", instance = "top.observer", interface = "Module:top.observer:peek", kind = "rule", name = "top_body.0.call.0.1", output_type = "u32", owner = "top.observer", params = [{consume = false, id = "v1", name = "q", targets = ["top.source"], type = "queue<u32>"}], public = true, result = "u32", rule = "peek"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32 loc(#loc12)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc13)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.observer_body", owner = "top.observer", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":25:43)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":31:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:24)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:16)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":40:22)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":39:42)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":39:18)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":40:12)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":41:24)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":42:25)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":15:16)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":15:9)

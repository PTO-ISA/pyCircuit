module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "DualChannel", type_order = [], types = {}}} {
  func.func @ac_init_top.source_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.source_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.source() -> tuple<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.source", params = [], result = "tuple<u32>"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc1)
    %0 = acir.aggregate(%c7_i32) {acir.source_type = "tuple<u32>"} : (i32) -> tuple<i32>
    return %0 : tuple<i32>
  }
  func.func @ac_init_top.left_out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.left_out() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.right_out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.right_out() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  acir.module @top attributes {acir.instance = {definition = "src_dual_channel_3c0614f6.DualChannel", name = "top", parent, rules = ["left", "right"]}} {
    acir.contract {relations = [], rules = ["left", "right"]}
    acir.queue @top.source : i32 {capacity_fn = "ac_init_top.source_cap", initial_fn = "ac_init_top.source", name = "top.source", type = "u32"}
    acir.queue @top.left_out : i32 {capacity_fn = "ac_init_top.left_out_cap", initial_fn = "ac_init_top.left_out", name = "top.left_out", type = "u32"}
    acir.queue @top.right_out : i32 {capacity_fn = "ac_init_top.right_out_cap", initial_fn = "ac_init_top.right_out", name = "top.right_out", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<i32>
    func.func private @ac_init_top.left_out_cap() -> i32
    func.func private @ac_init_top.left_out() -> tuple<>
    func.func private @ac_init_top.right_out_cap() -> i32
    func.func private @ac_init_top.right_out() -> tuple<>
    acir.rule @top_body.0.call.1.7 : (i32) -> i32 {acir.function = {contract = "Module:top", definition = "left", instance = "top", interface = "Module:top:left", kind = "rule", name = "top_body.0.call.1.7", output_type = "queue<u32>", owner = "top", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "u32", rule = "left"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %1 = "acir.optional"(%arg0, %true_0) {acir.source_type = "u32"} : (i32, i1) -> i32
      acir.return %1 : i32 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.2.9 : (i32) -> i32 {acir.function = {contract = "Module:top", definition = "right", instance = "top", interface = "Module:top:right", kind = "rule", name = "top_body.0.call.2.9", output_type = "queue<u32>", owner = "top", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "u32", rule = "right"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc2)
      %1 = arith.addi %arg0, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc3)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %2 = "acir.optional"(%1, %true_0) {acir.source_type = "u32"} : (i32, i1) -> i32
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.source {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.rule.call @top.observer::@top_body.0.call.0.2(%0) {acir.id = "v3", acir.source_type = "u32", contract = "Module:top.observer", instance = "top.observer", interface = "Module:top.observer:peek", role = "rule", rule = "peek"} : (!acir.queue<i32>) -> i32 loc(#loc4)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc5)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc4)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc6)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc7)
      %4 = acir.ref @top::@top.left_out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %5 = acir.ref @top::@top.right_out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      scf.if %3 {
        %6 = acir.rule.call @top::@top_body.0.call.1.7(%2) {acir.id = "v8", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:left", output_binding = @top::@top.left_out, role = "rule", rule = "left"} : (i32) -> i32 loc(#loc8)
        %7 = acir.rule.call @top::@top_body.0.call.2.9(%2) {acir.id = "v10", acir.source_type = "u32", contract = "Module:top", instance = "top", interface = "Module:top:right", output_binding = @top::@top.right_out, role = "rule", rule = "right"} : (i32) -> i32 loc(#loc9)
      } else {
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.observer attributes {acir.instance = {definition = "src_dual_channel_3c0614f6.Observer", name = "top.observer", parent = "top", rules = ["peek"]}} {
    acir.contract {relations = [], rules = ["peek"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<i32>
    func.func private @ac_init_top.left_out_cap() -> i32
    func.func private @ac_init_top.left_out() -> tuple<>
    func.func private @ac_init_top.right_out_cap() -> i32
    func.func private @ac_init_top.right_out() -> tuple<>
    acir.rule @top_body.0.call.0.2 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.observer", definition = "peek", instance = "top.observer", interface = "Module:top.observer:peek", kind = "rule", name = "top_body.0.call.0.2", output_type = "u32", owner = "top.observer", params = [{id = "v1", name = "q", targets = ["top.source"], type = "queue<u32>"}], public = true, result = "u32", rule = "peek"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32 loc(#loc10)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %2 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %2 {acir.source_type = "void"}
      acir.return %1 : i32 {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":22:43)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":32:24)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":32:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:18)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":35:42)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":36:22)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":36:12)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":37:24)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":38:25)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":15:16)

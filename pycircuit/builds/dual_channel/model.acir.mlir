module attributes {acir.model = {config = [], constants = {}, exports = {left_out = "left_out", right_out = "right_out", source = "source"}, format_version = 7 : i64, top = "DualChannel", type_order = [], types = {}}} {
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
  acir.module @observer attributes {acir.instance = {contract = "Module:observer", definition = "src_dual_channel_3c0614f6.Observer", inputs = {}, interfaces = {peek = "Module:observer:peek"}, members = {}, methods = ["peek"], name = "observer", outputs, parent = "top"}} {
    acir.interface @peek {implementations = [@observer::@top_Work.0.call.0.1, @observer::@ac_summary_observer.peek.0.call.1.1], orders = []}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<i32>
    func.func private @ac_init_left_out_cap() -> i32
    func.func private @ac_init_left_out() -> !acir.vector<i32>
    func.func private @ac_init_right_out_cap() -> i32
    func.func private @ac_init_right_out() -> !acir.vector<i32>
    acir.method @top_Work.0.call.0.1 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:observer", definition = "peek", instance = "observer", interface = "Module:observer:peek", kind = "method", method = "peek", name = "top_Work.0.call.0.1", params = [{consume = false, id = "v1", name = "q", targets = ["source"], type = "queue<u32>"}], result = "u32"}, interface_ref = @observer::@peek} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32 loc(#loc2)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc3)
    }
    acir.method @ac_summary_observer.peek.0.call.1.1 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:observer", definition = "peek", instance = "observer", interface = "Module:observer:peek", kind = "method", method = "peek", name = "ac_summary_observer.peek.0.call.1.1", params = [{consume = false, id = "v1", name = "q", targets = [], type = "queue<u32>"}], result = "u32", summary = false}, interface_ref = @observer::@peek} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32 loc(#loc2)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc3)
    }
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_dual_channel_3c0614f6.DualChannel", inputs = {}, interfaces = {}, members = {left_out = {resources = ["left_out"], type = "queue<u32>", value}, observer = {instance = "observer"}, right_out = {resources = ["right_out"], type = "queue<u32>", value}, source = {resources = ["source"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["left_out"], type = "queue<u32>", value}, {resources = ["right_out"], type = "queue<u32>", value}], parent}} {
    acir.instance @observer = @observer
    acir.queue @source : i32 {capacity_fn = "ac_init_source_cap", initial_fn = "ac_init_source", name = "source", type = "u32"}
    acir.queue @left_out : i32 {capacity_fn = "ac_init_left_out_cap", initial_fn = "ac_init_left_out", name = "left_out", type = "u32"}
    acir.queue @right_out : i32 {capacity_fn = "ac_init_right_out_cap", initial_fn = "ac_init_right_out", name = "right_out", type = "u32"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<i32>
    func.func private @ac_init_left_out_cap() -> i32
    func.func private @ac_init_left_out() -> !acir.vector<i32>
    func.func private @ac_init_right_out_cap() -> i32
    func.func private @ac_init_right_out() -> !acir.vector<i32>
    acir.rule @top.left : (i32) -> () {acir.function = {kind = "rule", name = "top.left", owner = "top", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", rule = "left"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: i32):
      acir.return %arg0 : i32 {acir.id = "v4", acir.source_type = "void", slots = [0]} loc(#loc4)
    }
    acir.rule @top.right : (i32) -> () {acir.function = {kind = "rule", name = "top.right", owner = "top", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", rule = "right"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: i32):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc5)
      %0 = arith.addi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc6)
      acir.return %0 : i32 {acir.id = "v6", acir.source_type = "void", slots = [0]} loc(#loc7)
    }
    acir.work attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc8)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc9)
      %0 = acir.ref @top::@source {acir.id = "v1", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc10)
      %1 = acir.method.call @observer::@top_Work.0.call.0.1(%0) {acir.id = "v2", acir.source_type = "u32", contract = "Module:observer", instance = "observer", interface = "Module:observer:peek", method = "peek", role = "method"} : (!acir.queue<i32>) -> i32 loc(#loc11)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc11)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc12)
      scf.if %3 {
        acir.rule.call @top::@top.left(%2) {acir.id = "v8", acir.source_type = "void", output_bindings = [@top::@left_out]} : (i32) -> () loc(#loc13)
        acir.rule.call @top::@top.right(%2) {acir.id = "v10", acir.source_type = "void", output_bindings = [@top::@right_out]} : (i32) -> () loc(#loc14)
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":18:39)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":12:16)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":12:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":24:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":28:24)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":28:16)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":28:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":31:18)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":30:38)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":18:14)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":30:14)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":31:8)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":32:20)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":33:21)


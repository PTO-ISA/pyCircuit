module attributes {acir.model = {config = [], constants = {}, exports = {left = "left", right = "right"}, format_version = 7 : i64, top = "QueueRing", type_order = [], types = {}}} {
  func.func @ac_init_left_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_left_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_left() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_left", params = [], result = "vector<u32>"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.aggregate(%c1_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_right_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_right_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_right() -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_right", params = [], result = "vector<u32>"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %0 = acir.aggregate(%c2_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_dependency_cycles_63d6eeaf.QueueRing", inputs = {}, interfaces = {}, members = {left = {resources = ["left"], type = "queue<u32>", value}, right = {resources = ["right"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["left"], type = "queue<u32>", value}, {resources = ["right"], type = "queue<u32>", value}], parent}} {
    acir.queue @left : i32 {capacity_fn = "ac_init_left_cap", initial_fn = "ac_init_left", name = "left", type = "u32"}
    acir.queue @right : i32 {capacity_fn = "ac_init_right_cap", initial_fn = "ac_init_right", name = "right", type = "u32"}
    func.func private @ac_init_left_cap() -> i32
    func.func private @ac_init_left() -> !acir.vector<i32>
    func.func private @ac_init_right_cap() -> i32
    func.func private @ac_init_right() -> !acir.vector<i32>
    acir.rule @top.forward : (!acir.queue<i32>) -> () {acir.function = {kind = "rule", name = "top.forward", owner = "top", params = [{consume = true, id = "v1", name = "source", targets = ["left"], type = "queue<u32>"}], result = "void", rule = "forward"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void", slots = [0]}
    }
    acir.rule @top.backward : (!acir.queue<i32>) -> () {acir.function = {kind = "rule", name = "top.backward", owner = "top", params = [{consume = true, id = "v1", name = "source", targets = ["right"], type = "queue<u32>"}], result = "void", rule = "backward"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void", slots = [0]}
    }
    acir.work attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
      %0 = acir.ref @top::@left {acir.id = "v1", acir.source_type = "queue<u32>"} : !acir.queue<i32>
      acir.rule.call @top::@top.forward(%0) {acir.id = "v2", acir.source_type = "void", output_bindings = [@top::@right]} : (!acir.queue<i32>) -> ()
      %1 = acir.ref @top::@right {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32>
      acir.rule.call @top::@top.backward(%1) {acir.id = "v4", acir.source_type = "void", output_bindings = [@top::@left]} : (!acir.queue<i32>) -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
}


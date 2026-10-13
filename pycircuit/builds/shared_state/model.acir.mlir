module attributes {acir.config = {choose_left = true}, acir.model = {config = [{name = "choose_left", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "SharedState", type_order = [], types = {}}} {
  func.func @ac_init_top.first_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.first_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.first(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.first", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc1)
    %0 = acir.aggregate(%c3_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.second_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.second_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.second(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.second", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32 loc(#loc2)
    %0 = acir.aggregate(%c5_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.third_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.third_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.third(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.third", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc3)
    %0 = acir.aggregate(%c9_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.left_out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.left_out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.right_out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.right_out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.independent_out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.independent_out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.independent_out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.independent_out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.shared.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.shared.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.separate.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.separate.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.SharedState", inputs = {}, members = {}, name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.first : i32 {capacity_fn = "ac_init_top.first_cap", initial_fn = "ac_init_top.first", name = "top.first", type = "u32"}
    acir.queue @top.second : i32 {capacity_fn = "ac_init_top.second_cap", initial_fn = "ac_init_top.second", name = "top.second", type = "u32"}
    acir.queue @top.third : i32 {capacity_fn = "ac_init_top.third_cap", initial_fn = "ac_init_top.third", name = "top.third", type = "u32"}
    acir.queue @top.left_out : i32 {capacity_fn = "ac_init_top.left_out_cap", initial_fn = "ac_init_top.left_out", name = "top.left_out", type = "u32"}
    acir.queue @top.right_out : i32 {capacity_fn = "ac_init_top.right_out_cap", initial_fn = "ac_init_top.right_out", name = "top.right_out", type = "u32"}
    acir.queue @top.independent_out : i32 {capacity_fn = "ac_init_top.independent_out_cap", initial_fn = "ac_init_top.independent_out", name = "top.independent_out", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc5)
      %0 = acir.config "choose_left" {acir.id = "v2", acir.source_type = "bool"} : i1 loc(#loc6)
      %1 = acir.export %0 {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
      %2 = acir.unary %0 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc7)
      %3 = acir.export %2 {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      %4 = acir.export %true {acir.id = "v9", acir.source_type = "bool"} : i1 -> i1
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.shared attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, members = {}, name = "top.shared", parent = "top", rules = ["add"]}} {
    acir.contract {relations = [], rules = ["add"]}
    acir.cell @top.shared.total : i32 {initial_fn = "ac_init_top.shared.total", name = "top.shared.total", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.left_body.0.call.0.3.0.call.0.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.shared", definition = "add", instance = "top.shared", interface = "Module:top.shared:add", kind = "rule", name = "top.left_body.0.call.0.3.0.call.0.3", output_type = "u32", owner = "top.shared", params = [{consume = false, id = "v1", name = "message", targets = ["top.first"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc8)
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc9)
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc8)
      acir.cell.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc10)
      %4 = acir.queue.peek %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc11)
      acir.return %4 : i32 {acir.id = "v11", acir.source_type = "void"} loc(#loc12)
    }
    acir.rule @top.right_body.0.call.2.3.0.call.2.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.shared", definition = "add", instance = "top.shared", interface = "Module:top.shared:add", kind = "rule", name = "top.right_body.0.call.2.3.0.call.2.3", output_type = "u32", owner = "top.shared", params = [{consume = false, id = "v1", name = "message", targets = ["top.second"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc8)
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc9)
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc8)
      acir.cell.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc10)
      %4 = acir.queue.peek %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc11)
      acir.return %4 : i32 {acir.id = "v11", acir.source_type = "void"} loc(#loc12)
    }
  }
  acir.module @top.separate attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, members = {}, name = "top.separate", parent = "top", rules = ["add"]}} {
    acir.contract {relations = [], rules = ["add"]}
    acir.cell @top.separate.total : i32 {initial_fn = "ac_init_top.separate.total", name = "top.separate.total", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.independent_body.0.call.4.3.0.call.4.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.separate", definition = "add", instance = "top.separate", interface = "Module:top.separate:add", kind = "rule", name = "top.independent_body.0.call.4.3.0.call.4.3", output_type = "u32", owner = "top.separate", params = [{consume = false, id = "v1", name = "message", targets = ["top.third"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @top.separate::@top.separate.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc8)
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc9)
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc8)
      acir.cell.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc10)
      %4 = acir.queue.peek %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc11)
      acir.return %4 : i32 {acir.id = "v11", acir.source_type = "void"} loc(#loc12)
    }
  }
  acir.module @top.left attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", inputs = {}, members = {}, name = "top.left", parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.left_body.0.call.0.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.left", definition = "transfer", instance = "top.left", interface = "Module:top.left:transfer", kind = "rule", name = "top.left_body.0.call.0.3", output_type = "queue<u32>", owner = "top.left", params = [{consume = false, id = "v1", name = "message", targets = ["top.first"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.rule.call @top.shared::@top.left_body.0.call.0.3.0.call.0.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.shared", instance = "top.shared", interface = "Module:top.shared:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc13)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc14)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.left_body", owner = "top.left", params = [], result = "void"}} {
      %0 = acir.external "top_body" "v3" {acir.source_type = "bool"} : i1
      cf.cond_br %0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %1 = acir.ref @top::@top.first {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %2 = acir.rule.call @top.left::@top.left_body.0.call.0.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:transfer", output_binding = @top::@top.left_out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc15)
      acir.body.end {acir.id = "v6", acir.source_type = "void"} loc(#loc16)
    ^bb2:  // pred: ^bb0
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.right attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", inputs = {}, members = {}, name = "top.right", parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.right_body.0.call.2.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.right", definition = "transfer", instance = "top.right", interface = "Module:top.right:transfer", kind = "rule", name = "top.right_body.0.call.2.3", output_type = "queue<u32>", owner = "top.right", params = [{consume = false, id = "v1", name = "message", targets = ["top.second"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.rule.call @top.shared::@top.right_body.0.call.2.3.0.call.2.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.shared", instance = "top.shared", interface = "Module:top.shared:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc13)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc14)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.right_body", owner = "top.right", params = [], result = "void"}} {
      %0 = acir.external "top_body" "v6" {acir.source_type = "bool"} : i1
      cf.cond_br %0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %1 = acir.ref @top::@top.second {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %2 = acir.rule.call @top.right::@top.right_body.0.call.2.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:transfer", output_binding = @top::@top.right_out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc15)
      acir.body.end {acir.id = "v6", acir.source_type = "void"} loc(#loc16)
    ^bb2:  // pred: ^bb0
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.independent attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", inputs = {}, members = {}, name = "top.independent", parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.independent_body.0.call.4.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.independent", definition = "transfer", instance = "top.independent", interface = "Module:top.independent:transfer", kind = "rule", name = "top.independent_body.0.call.4.3", output_type = "queue<u32>", owner = "top.independent", params = [{consume = false, id = "v1", name = "message", targets = ["top.third"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.rule.call @top.separate::@top.independent_body.0.call.4.3.0.call.4.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.separate", instance = "top.separate", interface = "Module:top.separate:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc13)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc14)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.independent_body", owner = "top.independent", params = [], result = "void"}} {
      %0 = acir.external "top_body" "v9" {acir.source_type = "bool"} : i1
      cf.cond_br %0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %1 = acir.ref @top::@top.third {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %2 = acir.rule.call @top.independent::@top.independent_body.0.call.4.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.independent", instance = "top.independent", interface = "Module:top.independent:transfer", output_binding = @top::@top.independent_out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc15)
      acir.body.end {acir.id = "v6", acir.source_type = "void"} loc(#loc16)
    ^bb2:  // pred: ^bb0
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":33:42)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":34:43)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":35:42)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":8:41)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":48:56)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":32:18)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":47:43)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:17)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:25)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":13:16)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":13:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":23:16)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":23:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":27:20)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":27:13)

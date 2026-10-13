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
  func.func @ac_init_top.shared.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.shared.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.separate.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.separate.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.left.out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.left.out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.left.out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.left.out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.right.out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.right.out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.right.out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.right.out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.independent.out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.independent.out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.independent.out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.independent.out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  acir.module @top attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.SharedState", inputs = {}, members = {}, name = "top", outputs, parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.first : i32 {capacity_fn = "ac_init_top.first_cap", initial_fn = "ac_init_top.first", name = "top.first", type = "u32"}
    acir.queue @top.second : i32 {capacity_fn = "ac_init_top.second_cap", initial_fn = "ac_init_top.second", name = "top.second", type = "u32"}
    acir.queue @top.third : i32 {capacity_fn = "ac_init_top.third_cap", initial_fn = "ac_init_top.third", name = "top.third", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    func.func private @ac_init_top.left.out_cap(i1) -> i32
    func.func private @ac_init_top.left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right.out_cap(i1) -> i32
    func.func private @ac_init_top.right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent.out_cap(i1) -> i32
    func.func private @ac_init_top.independent.out(i1) -> !acir.vector<i32>
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc5)
      %0 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc6)
      %2 = acir.ref @top.separate::@top.separate.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %3 = acir.cell.read %2 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc7)
      %4 = acir.config "choose_left" {acir.id = "v6", acir.source_type = "bool"} : i1 loc(#loc8)
      %5 = acir.export %4 {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1
      %6 = acir.ref @top.left::@top.left.out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %7 = acir.queue.peek %6 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32 loc(#loc9)
      %8 = acir.unary %4 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc10)
      %9 = acir.export %8 {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
      %10 = acir.ref @top.right::@top.right.out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %11 = acir.queue.peek %10 {acir.id = "v14", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32 loc(#loc11)
      %12 = acir.export %true {acir.id = "v17", acir.source_type = "bool"} : i1 -> i1
      %13 = acir.ref @top.independent::@top.independent.out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %14 = acir.queue.peek %13 {acir.id = "v19", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32 loc(#loc12)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.shared attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, members = {}, name = "top.shared", outputs, parent = "top", rules = ["add"]}} {
    acir.contract {relations = [], rules = ["add"]}
    acir.cell @top.shared.total : i32 {initial_fn = "ac_init_top.shared.total", name = "top.shared.total", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    func.func private @ac_init_top.left.out_cap(i1) -> i32
    func.func private @ac_init_top.left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right.out_cap(i1) -> i32
    func.func private @ac_init_top.right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent.out_cap(i1) -> i32
    func.func private @ac_init_top.independent.out(i1) -> !acir.vector<i32>
    acir.rule @top.left_body.0.call.0.3.0.call.0.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.shared", definition = "add", instance = "top.shared", interface = "Module:top.shared:add", kind = "rule", name = "top.left_body.0.call.0.3.0.call.0.3", output_type = "u32", owner = "top.shared", params = [{consume = false, id = "v1", name = "message", targets = ["top.first"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc13)
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc14)
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc13)
      acir.cell.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc15)
      %4 = acir.queue.peek %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc16)
      acir.return %4 : i32 {acir.id = "v11", acir.source_type = "void"} loc(#loc17)
    }
    acir.rule @top.right_body.0.call.2.3.0.call.2.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.shared", definition = "add", instance = "top.shared", interface = "Module:top.shared:add", kind = "rule", name = "top.right_body.0.call.2.3.0.call.2.3", output_type = "u32", owner = "top.shared", params = [{consume = false, id = "v1", name = "message", targets = ["top.second"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc13)
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc14)
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc13)
      acir.cell.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc15)
      %4 = acir.queue.peek %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc16)
      acir.return %4 : i32 {acir.id = "v11", acir.source_type = "void"} loc(#loc17)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.shared_body", owner = "top.shared", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.separate attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, members = {}, name = "top.separate", outputs, parent = "top", rules = ["add"]}} {
    acir.contract {relations = [], rules = ["add"]}
    acir.cell @top.separate.total : i32 {initial_fn = "ac_init_top.separate.total", name = "top.separate.total", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    func.func private @ac_init_top.left.out_cap(i1) -> i32
    func.func private @ac_init_top.left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right.out_cap(i1) -> i32
    func.func private @ac_init_top.right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent.out_cap(i1) -> i32
    func.func private @ac_init_top.independent.out(i1) -> !acir.vector<i32>
    acir.rule @top.independent_body.0.call.4.3.0.call.4.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.separate", definition = "add", instance = "top.separate", interface = "Module:top.separate:add", kind = "rule", name = "top.independent_body.0.call.4.3.0.call.4.3", output_type = "u32", owner = "top.separate", params = [{consume = false, id = "v1", name = "message", targets = ["top.third"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @top.separate::@top.separate.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc13)
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc14)
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc13)
      acir.cell.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc15)
      %4 = acir.queue.peek %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc16)
      acir.return %4 : i32 {acir.id = "v11", acir.source_type = "void"} loc(#loc17)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.separate_body", owner = "top.separate", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.left attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", inputs = {}, members = {}, name = "top.left", outputs, parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    acir.queue @top.left.out : i32 {capacity_fn = "ac_init_top.left.out_cap", initial_fn = "ac_init_top.left.out", name = "top.left.out", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    func.func private @ac_init_top.left.out_cap(i1) -> i32
    func.func private @ac_init_top.left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right.out_cap(i1) -> i32
    func.func private @ac_init_top.right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent.out_cap(i1) -> i32
    func.func private @ac_init_top.independent.out(i1) -> !acir.vector<i32>
    acir.rule @top.left_body.0.call.0.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.left", definition = "transfer", instance = "top.left", interface = "Module:top.left:transfer", kind = "rule", name = "top.left_body.0.call.0.3", output_type = "queue<u32>", owner = "top.left", params = [{consume = false, id = "v1", name = "message", targets = ["top.first"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.rule.call @top.shared::@top.left_body.0.call.0.3.0.call.0.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.shared", instance = "top.shared", interface = "Module:top.shared:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc18)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc19)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.left_body", owner = "top.left", params = [], result = "void"}} {
      %0 = acir.external "top_body" "v7" {acir.source_type = "bool"} : i1
      scf.if %0 {
        %1 = acir.ref @top::@top.first {acir.source_type = "queue<u32>"} : !acir.queue<i32>
        %2 = acir.rule.call @top.left::@top.left_body.0.call.0.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:transfer", output_binding = @top.left::@top.left.out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc20)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.right attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", inputs = {}, members = {}, name = "top.right", outputs, parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    acir.queue @top.right.out : i32 {capacity_fn = "ac_init_top.right.out_cap", initial_fn = "ac_init_top.right.out", name = "top.right.out", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    func.func private @ac_init_top.left.out_cap(i1) -> i32
    func.func private @ac_init_top.left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right.out_cap(i1) -> i32
    func.func private @ac_init_top.right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent.out_cap(i1) -> i32
    func.func private @ac_init_top.independent.out(i1) -> !acir.vector<i32>
    acir.rule @top.right_body.0.call.2.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.right", definition = "transfer", instance = "top.right", interface = "Module:top.right:transfer", kind = "rule", name = "top.right_body.0.call.2.3", output_type = "queue<u32>", owner = "top.right", params = [{consume = false, id = "v1", name = "message", targets = ["top.second"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.rule.call @top.shared::@top.right_body.0.call.2.3.0.call.2.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.shared", instance = "top.shared", interface = "Module:top.shared:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc18)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc19)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.right_body", owner = "top.right", params = [], result = "void"}} {
      %0 = acir.external "top_body" "v12" {acir.source_type = "bool"} : i1
      scf.if %0 {
        %1 = acir.ref @top::@top.second {acir.source_type = "queue<u32>"} : !acir.queue<i32>
        %2 = acir.rule.call @top.right::@top.right_body.0.call.2.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:transfer", output_binding = @top.right::@top.right.out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc20)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.independent attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", inputs = {}, members = {}, name = "top.independent", outputs, parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    acir.queue @top.independent.out : i32 {capacity_fn = "ac_init_top.independent.out_cap", initial_fn = "ac_init_top.independent.out", name = "top.independent.out", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    func.func private @ac_init_top.left.out_cap(i1) -> i32
    func.func private @ac_init_top.left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.right.out_cap(i1) -> i32
    func.func private @ac_init_top.right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.independent.out_cap(i1) -> i32
    func.func private @ac_init_top.independent.out(i1) -> !acir.vector<i32>
    acir.rule @top.independent_body.0.call.4.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.independent", definition = "transfer", instance = "top.independent", interface = "Module:top.independent:transfer", kind = "rule", name = "top.independent_body.0.call.4.3", output_type = "queue<u32>", owner = "top.independent", params = [{consume = false, id = "v1", name = "message", targets = ["top.third"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.rule.call @top.separate::@top.independent_body.0.call.4.3.0.call.4.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.separate", instance = "top.separate", interface = "Module:top.separate:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc18)
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void"} loc(#loc19)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.independent_body", owner = "top.independent", params = [], result = "void"}} {
      %0 = acir.external "top_body" "v17" {acir.source_type = "bool"} : i1
      scf.if %0 {
        %1 = acir.ref @top::@top.third {acir.source_type = "queue<u32>"} : !acir.queue<i32>
        %2 = acir.rule.call @top.independent::@top.independent_body.0.call.4.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.independent", instance = "top.independent", interface = "Module:top.independent:transfer", output_binding = @top.independent::@top.independent.out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc20)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":37:42)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":38:43)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":39:42)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":8:41)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":51:38)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":47:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":48:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":36:18)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":49:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":50:31)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":50:9)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":51:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:17)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:25)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:9)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":13:16)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":13:9)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":26:16)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":26:9)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":30:19)

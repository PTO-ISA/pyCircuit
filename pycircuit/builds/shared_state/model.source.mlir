module attributes {acir.config = {choose_left = true}, acir.model = {config = [{name = "choose_left", type = "bool"}], constants = {}, format_version = 9 : i64, top = "SharedState", type_order = [], types = {}}} {
  func.func @ac_init_top.first_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.first_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.first(%arg0: i1) -> tuple<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.first", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "tuple<u32>"}} {
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc1)
    %0 = acir.aggregate(%c3_i32) {acir.source_type = "tuple<u32>"} : (i32) -> tuple<i32>
    return %0 : tuple<i32>
  }
  func.func @ac_init_top.second_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.second_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.second(%arg0: i1) -> tuple<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.second", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "tuple<u32>"}} {
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32 loc(#loc2)
    %0 = acir.aggregate(%c5_i32) {acir.source_type = "tuple<u32>"} : (i32) -> tuple<i32>
    return %0 : tuple<i32>
  }
  func.func @ac_init_top.third_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.third_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.third(%arg0: i1) -> tuple<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.third", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "tuple<u32>"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc3)
    %0 = acir.aggregate(%c9_i32) {acir.source_type = "tuple<u32>"} : (i32) -> tuple<i32>
    return %0 : tuple<i32>
  }
  func.func @ac_init_top.left_out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.left_out(%arg0: i1) -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.left_out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.right_out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.right_out(%arg0: i1) -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.right_out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.independent_out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.independent_out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.independent_out(%arg0: i1) -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.independent_out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.shared.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.shared.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.separate.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.separate.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.SharedState", name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.first : i32 {capacity_fn = "ac_init_top.first_cap", initial_fn = "ac_init_top.first", name = "top.first", type = "u32"}
    acir.queue @top.second : i32 {capacity_fn = "ac_init_top.second_cap", initial_fn = "ac_init_top.second", name = "top.second", type = "u32"}
    acir.queue @top.third : i32 {capacity_fn = "ac_init_top.third_cap", initial_fn = "ac_init_top.third", name = "top.third", type = "u32"}
    acir.queue @top.left_out : i32 {capacity_fn = "ac_init_top.left_out_cap", initial_fn = "ac_init_top.left_out", name = "top.left_out", type = "u32"}
    acir.queue @top.right_out : i32 {capacity_fn = "ac_init_top.right_out_cap", initial_fn = "ac_init_top.right_out", name = "top.right_out", type = "u32"}
    acir.queue @top.independent_out : i32 {capacity_fn = "ac_init_top.independent_out_cap", initial_fn = "ac_init_top.independent_out", name = "top.independent_out", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> tuple<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> tuple<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> tuple<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> tuple<>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> tuple<>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> tuple<>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.first {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.config "choose_left" {acir.id = "v3", acir.source_type = "bool"} : i1 loc(#loc5)
      %2 = acir.export %1 {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1
      acir.module.call @top.left loc(#loc6)
      %3 = acir.ref @top::@top.second {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %4 = acir.unary %1 "not" {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1 loc(#loc7)
      %5 = acir.export %4 {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
      acir.module.call @top.right loc(#loc8)
      %6 = acir.ref @top::@top.third {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc9)
      %7 = acir.export %true_0 {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
      acir.module.call @top.independent loc(#loc10)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.shared attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Accumulator", name = "top.shared", parent = "top", rules = ["add"]}} {
    acir.contract {relations = [], rules = ["add"]}
    acir.cell @top.shared.total : i32 {initial_fn = "ac_init_top.shared.total", name = "top.shared.total", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> tuple<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> tuple<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> tuple<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> tuple<>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> tuple<>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> tuple<>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.left_body.0.call.0.3.0.call.0.6 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.shared", definition = "add", instance = "top.shared", interface = "Module:top.shared:add", kind = "rule", name = "top.left_body.0.call.0.3.0.call.0.6", output_type = "u32", owner = "top.shared", params = [{id = "v1", name = "message", targets = ["top.first"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc11)
      %3 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc12)
      %4 = arith.addi %2, %3 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc11)
      %5 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc13)
      %6 = acir.queue.peek %arg0 {acir.id = "v13", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc14)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %6 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.right_body.0.call.2.3.0.call.2.6 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.shared", definition = "add", instance = "top.shared", interface = "Module:top.shared:add", kind = "rule", name = "top.right_body.0.call.2.3.0.call.2.6", output_type = "u32", owner = "top.shared", params = [{id = "v1", name = "message", targets = ["top.second"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc11)
      %3 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc12)
      %4 = arith.addi %2, %3 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc11)
      %5 = acir.ref @top.shared::@top.shared.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc13)
      %6 = acir.queue.peek %arg0 {acir.id = "v13", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc14)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %6 : i32 {acir.source_type = "void"}
    }
  }
  acir.module @top.separate attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Accumulator", name = "top.separate", parent = "top", rules = ["add"]}} {
    acir.contract {relations = [], rules = ["add"]}
    acir.cell @top.separate.total : i32 {initial_fn = "ac_init_top.separate.total", name = "top.separate.total", type = "u32"}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> tuple<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> tuple<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> tuple<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> tuple<>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> tuple<>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> tuple<>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.independent_body.0.call.4.3.0.call.4.6 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.separate", definition = "add", instance = "top.separate", interface = "Module:top.separate:add", kind = "rule", name = "top.independent_body.0.call.4.3.0.call.4.6", output_type = "u32", owner = "top.separate", params = [{id = "v1", name = "message", targets = ["top.third"], type = "queue<u32>"}], public = true, result = "u32", rule = "add"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.separate::@top.separate.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc11)
      %3 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc12)
      %4 = arith.addi %2, %3 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc11)
      %5 = acir.ref @top.separate::@top.separate.total {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc13)
      %6 = acir.queue.peek %arg0 {acir.id = "v13", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc14)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %6 : i32 {acir.source_type = "void"}
    }
  }
  acir.module @top.left attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", name = "top.left", parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> tuple<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> tuple<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> tuple<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> tuple<>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> tuple<>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> tuple<>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.left_body.0.call.0.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.left", definition = "transfer", instance = "top.left", interface = "Module:top.left:transfer", kind = "rule", name = "top.left_body.0.call.0.3", output_type = "queue<u32>", owner = "top.left", params = [{id = "v1", name = "message", targets = ["top.first"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.rule.call @top.shared::@top.left_body.0.call.0.3.0.call.0.6(%arg0) {acir.id = "v7", acir.source_type = "u32", contract = "Module:top.shared", instance = "top.shared", interface = "Module:top.shared:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc15)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %2 = "acir.optional"(%1, %true_0) {acir.source_type = "u32"} : (i32, i1) -> i32
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.left_body", owner = "top.left", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.external "top_body" "v4" {acir.source_type = "bool"} : i1
      %1 = acir.ref @top::@top.first {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %2 = acir.ref @top::@top.left_out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %3 = scf.if %0 -> (i1) {
        %4 = acir.rule.call @top.left::@top.left_body.0.call.0.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:transfer", output_binding = @top::@top.left_out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc16)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.right attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", name = "top.right", parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> tuple<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> tuple<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> tuple<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> tuple<>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> tuple<>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> tuple<>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.right_body.0.call.2.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.right", definition = "transfer", instance = "top.right", interface = "Module:top.right:transfer", kind = "rule", name = "top.right_body.0.call.2.3", output_type = "queue<u32>", owner = "top.right", params = [{id = "v1", name = "message", targets = ["top.second"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.rule.call @top.shared::@top.right_body.0.call.2.3.0.call.2.6(%arg0) {acir.id = "v7", acir.source_type = "u32", contract = "Module:top.shared", instance = "top.shared", interface = "Module:top.shared:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc15)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %2 = "acir.optional"(%1, %true_0) {acir.source_type = "u32"} : (i32, i1) -> i32
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.right_body", owner = "top.right", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.external "top_body" "v8" {acir.source_type = "bool"} : i1
      %1 = acir.ref @top::@top.second {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %2 = acir.ref @top::@top.right_out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %3 = scf.if %0 -> (i1) {
        %4 = acir.rule.call @top.right::@top.right_body.0.call.2.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:transfer", output_binding = @top::@top.right_out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc16)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.independent attributes {acir.instance = {definition = "src_shared_state_16f7c6b8.Client", name = "top.independent", parent = "top", rules = ["transfer"]}} {
    acir.contract {relations = [], rules = ["transfer"]}
    func.func private @ac_init_top.first_cap(i1) -> i32
    func.func private @ac_init_top.first(i1) -> tuple<i32>
    func.func private @ac_init_top.second_cap(i1) -> i32
    func.func private @ac_init_top.second(i1) -> tuple<i32>
    func.func private @ac_init_top.third_cap(i1) -> i32
    func.func private @ac_init_top.third(i1) -> tuple<i32>
    func.func private @ac_init_top.left_out_cap(i1) -> i32
    func.func private @ac_init_top.left_out(i1) -> tuple<>
    func.func private @ac_init_top.right_out_cap(i1) -> i32
    func.func private @ac_init_top.right_out(i1) -> tuple<>
    func.func private @ac_init_top.independent_out_cap(i1) -> i32
    func.func private @ac_init_top.independent_out(i1) -> tuple<>
    func.func private @ac_init_top.shared.total(i1) -> i32
    func.func private @ac_init_top.separate.total(i1) -> i32
    acir.rule @top.independent_body.0.call.4.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:top.independent", definition = "transfer", instance = "top.independent", interface = "Module:top.independent:transfer", kind = "rule", name = "top.independent_body.0.call.4.3", output_type = "queue<u32>", owner = "top.independent", params = [{id = "v1", name = "message", targets = ["top.third"], type = "queue<u32>"}], public = true, result = "u32", rule = "transfer"}} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.rule.call @top.separate::@top.independent_body.0.call.4.3.0.call.4.6(%arg0) {acir.id = "v7", acir.source_type = "u32", contract = "Module:top.separate", instance = "top.separate", interface = "Module:top.separate:add", role = "rule", rule = "add"} : (!acir.queue<i32>) -> i32 loc(#loc15)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %2 = "acir.optional"(%1, %true_0) {acir.source_type = "u32"} : (i32, i1) -> i32
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.independent_body", owner = "top.independent", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.external "top_body" "v12" {acir.source_type = "bool"} : i1
      %1 = acir.ref @top::@top.third {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %2 = acir.ref @top::@top.independent_out {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %3 = scf.if %0 -> (i1) {
        %4 = acir.rule.call @top.independent::@top.independent_body.0.call.4.3(%1) {acir.id = "v4", acir.source_type = "u32", contract = "Module:top.independent", instance = "top.independent", interface = "Module:top.independent:transfer", output_binding = @top::@top.independent_out, role = "rule", rule = "transfer"} : (!acir.queue<i32>) -> i32 loc(#loc16)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":33:42)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":34:43)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":35:42)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":8:41)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":32:18)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":46:20)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":47:43)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":47:21)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":48:56)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":48:27)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:17)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:25)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":13:16)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":23:16)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":27:20)

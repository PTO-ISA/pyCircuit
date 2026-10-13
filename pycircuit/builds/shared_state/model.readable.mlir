module attributes {acir.model = {config = [{name = "choose_left", type = "bool"}], constants = {}, exports = {first = "first", second = "second", third = "third"}, format_version = 7 : i64, top = "SharedState", type_order = [], types = {}}} {
  func.func @ac_init_first_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_first_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_first(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_first", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = acir.aggregate(%c3_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_second_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_second_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_second(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_second", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %0 = acir.aggregate(%c5_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_third_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_third_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_third(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_third", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32
    %0 = acir.aggregate(%c9_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_shared.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_shared.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_separate.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_separate.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_left.out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_left.out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_left.out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_left.out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_right.out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_right.out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_right.out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_right.out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_independent.out_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_independent.out_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_independent.out(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_independent.out", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  acir.module @shared attributes {acir.instance = {contract = "Module:shared", definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, interfaces = {add = "Module:shared:add"}, members = {total = {resources = ["shared.total"], type = "cell<u32>", value}}, methods = ["add"], name = "shared", outputs = {resources = ["shared.total"], type = "cell<u32>", value}, parent = "top"}} {
    acir.interface @add {implementations = [@shared::@left.transfer.0.call.0.3, @shared::@right.transfer.0.call.1.3, @shared::@ac_summary_shared.add.0.call.3.1], orders = []}
    acir.cell @shared.total : i32 {initial_fn = "ac_init_shared.total", name = "shared.total", type = "u32"}
    func.func private @ac_init_first_cap(i1) -> i32
    func.func private @ac_init_first(i1) -> !acir.vector<i32>
    func.func private @ac_init_second_cap(i1) -> i32
    func.func private @ac_init_second(i1) -> !acir.vector<i32>
    func.func private @ac_init_third_cap(i1) -> i32
    func.func private @ac_init_third(i1) -> !acir.vector<i32>
    func.func private @ac_init_shared.total(i1) -> i32
    func.func private @ac_init_separate.total(i1) -> i32
    func.func private @ac_init_left.out_cap(i1) -> i32
    func.func private @ac_init_left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_right.out_cap(i1) -> i32
    func.func private @ac_init_right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_independent.out_cap(i1) -> i32
    func.func private @ac_init_independent.out(i1) -> !acir.vector<i32>
    acir.method @left.transfer.0.call.0.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:shared", definition = "add", instance = "shared", interface = "Module:shared:add", kind = "method", method = "add", name = "left.transfer.0.call.0.3", params = [{consume = true, id = "v1", name = "message", targets = ["first"], type = "queue<u32>"}], result = "u32"}, interface_ref = @shared::@add} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @shared::@shared.total {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32
      acir.cell.write %0, %3 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32
      %4 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      acir.return %4 : i32 {acir.id = "v10", acir.source_type = "void"}
    }
    acir.method @right.transfer.0.call.1.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:shared", definition = "add", instance = "shared", interface = "Module:shared:add", kind = "method", method = "add", name = "right.transfer.0.call.1.3", params = [{consume = true, id = "v1", name = "message", targets = ["second"], type = "queue<u32>"}], result = "u32"}, interface_ref = @shared::@add} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @shared::@shared.total {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32
      acir.cell.write %0, %3 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32
      %4 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      acir.return %4 : i32 {acir.id = "v10", acir.source_type = "void"}
    }
    acir.method @ac_summary_shared.add.0.call.3.1 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:shared", definition = "add", instance = "shared", interface = "Module:shared:add", kind = "method", method = "add", name = "ac_summary_shared.add.0.call.3.1", params = [{consume = false, id = "v1", name = "message", targets = [], type = "queue<u32>"}], result = "u32", summary = false}, interface_ref = @shared::@add} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @shared::@shared.total {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32
      acir.cell.write %0, %3 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32
      %4 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32
      acir.return %4 : i32 {acir.id = "v10", acir.source_type = "void"}
    }
  }
  acir.module @separate attributes {acir.instance = {contract = "Module:separate", definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, interfaces = {add = "Module:separate:add"}, members = {total = {resources = ["separate.total"], type = "cell<u32>", value}}, methods = ["add"], name = "separate", outputs = {resources = ["separate.total"], type = "cell<u32>", value}, parent = "top"}} {
    acir.interface @add {implementations = [@separate::@independent.transfer.0.call.2.3, @separate::@ac_summary_separate.add.0.call.4.1], orders = []}
    acir.cell @separate.total : i32 {initial_fn = "ac_init_separate.total", name = "separate.total", type = "u32"}
    func.func private @ac_init_first_cap(i1) -> i32
    func.func private @ac_init_first(i1) -> !acir.vector<i32>
    func.func private @ac_init_second_cap(i1) -> i32
    func.func private @ac_init_second(i1) -> !acir.vector<i32>
    func.func private @ac_init_third_cap(i1) -> i32
    func.func private @ac_init_third(i1) -> !acir.vector<i32>
    func.func private @ac_init_shared.total(i1) -> i32
    func.func private @ac_init_separate.total(i1) -> i32
    func.func private @ac_init_left.out_cap(i1) -> i32
    func.func private @ac_init_left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_right.out_cap(i1) -> i32
    func.func private @ac_init_right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_independent.out_cap(i1) -> i32
    func.func private @ac_init_independent.out(i1) -> !acir.vector<i32>
    acir.method @independent.transfer.0.call.2.3 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:separate", definition = "add", instance = "separate", interface = "Module:separate:add", kind = "method", method = "add", name = "independent.transfer.0.call.2.3", params = [{consume = true, id = "v1", name = "message", targets = ["third"], type = "queue<u32>"}], result = "u32"}, interface_ref = @separate::@add} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @separate::@separate.total {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32
      acir.cell.write %0, %3 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32
      %4 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32
      acir.return %4 : i32 {acir.id = "v10", acir.source_type = "void"}
    }
    acir.method @ac_summary_separate.add.0.call.4.1 : (!acir.queue<i32>) -> i32 {acir.function = {contract = "Module:separate", definition = "add", instance = "separate", interface = "Module:separate:add", kind = "method", method = "add", name = "ac_summary_separate.add.0.call.4.1", params = [{consume = false, id = "v1", name = "message", targets = [], type = "queue<u32>"}], result = "u32", summary = false}, interface_ref = @separate::@add} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.ref @separate::@separate.total {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.queue.peek %arg0 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32
      %3 = arith.addi %1, %2 {acir.id = "v7", acir.source_type = "u32"} : i32
      acir.cell.write %0, %3 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32
      %4 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.queue<i32> -> i32
      acir.return %4 : i32 {acir.id = "v10", acir.source_type = "void"}
    }
  }
  acir.module @left attributes {acir.instance = {contract = "Module:left", definition = "src_shared_state_16f7c6b8.Client", inputs = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, source = {resources = ["first"], type = "queue<u32>", value}}, interfaces = {}, members = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, out = {resources = ["left.out"], type = "queue<u32>", value}, source = {resources = ["first"], type = "queue<u32>", value}}, methods = [], name = "left", outputs = {resources = ["left.out"], type = "queue<u32>", value}, parent = "top"}} {
    acir.instance @accumulator = @shared
    acir.queue @left.out : i32 {capacity_fn = "ac_init_left.out_cap", initial_fn = "ac_init_left.out", name = "left.out", type = "u32"}
    func.func private @ac_init_first_cap(i1) -> i32
    func.func private @ac_init_first(i1) -> !acir.vector<i32>
    func.func private @ac_init_second_cap(i1) -> i32
    func.func private @ac_init_second(i1) -> !acir.vector<i32>
    func.func private @ac_init_third_cap(i1) -> i32
    func.func private @ac_init_third(i1) -> !acir.vector<i32>
    func.func private @ac_init_shared.total(i1) -> i32
    func.func private @ac_init_separate.total(i1) -> i32
    func.func private @ac_init_left.out_cap(i1) -> i32
    func.func private @ac_init_left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_right.out_cap(i1) -> i32
    func.func private @ac_init_right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_independent.out_cap(i1) -> i32
    func.func private @ac_init_independent.out(i1) -> !acir.vector<i32>
    acir.rule @left.transfer : (!acir.queue<i32>) -> () {acir.function = {kind = "rule", name = "left.transfer", owner = "left", params = [{consume = true, id = "v1", name = "message", targets = ["first"], type = "queue<u32>"}], result = "void", rule = "transfer"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.method.call @shared::@left.transfer.0.call.0.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:shared", instance = "shared", interface = "Module:shared:add", method = "add", role = "method"} : (!acir.queue<i32>) -> i32
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void", slots = [0]}
    }
    acir.work attributes {acir.function = {kind = "work", name = "left_Work", owner = "left", params = [], result = "void"}} {
      %0 = acir.config "choose_left" {acir.id = "v1", acir.source_type = "bool"} : i1
      %1 = acir.ref @top::@first {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32>
      scf.if %0 {
        acir.rule.call @left::@left.transfer(%1) {acir.id = "v4", acir.source_type = "void", output_bindings = [@left::@left.out]} : (!acir.queue<i32>) -> ()
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @right attributes {acir.instance = {contract = "Module:right", definition = "src_shared_state_16f7c6b8.Client", inputs = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, source = {resources = ["second"], type = "queue<u32>", value}}, interfaces = {}, members = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, out = {resources = ["right.out"], type = "queue<u32>", value}, source = {resources = ["second"], type = "queue<u32>", value}}, methods = [], name = "right", outputs = {resources = ["right.out"], type = "queue<u32>", value}, parent = "top"}} {
    acir.instance @accumulator = @shared
    acir.queue @right.out : i32 {capacity_fn = "ac_init_right.out_cap", initial_fn = "ac_init_right.out", name = "right.out", type = "u32"}
    func.func private @ac_init_first_cap(i1) -> i32
    func.func private @ac_init_first(i1) -> !acir.vector<i32>
    func.func private @ac_init_second_cap(i1) -> i32
    func.func private @ac_init_second(i1) -> !acir.vector<i32>
    func.func private @ac_init_third_cap(i1) -> i32
    func.func private @ac_init_third(i1) -> !acir.vector<i32>
    func.func private @ac_init_shared.total(i1) -> i32
    func.func private @ac_init_separate.total(i1) -> i32
    func.func private @ac_init_left.out_cap(i1) -> i32
    func.func private @ac_init_left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_right.out_cap(i1) -> i32
    func.func private @ac_init_right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_independent.out_cap(i1) -> i32
    func.func private @ac_init_independent.out(i1) -> !acir.vector<i32>
    acir.rule @right.transfer : (!acir.queue<i32>) -> () {acir.function = {kind = "rule", name = "right.transfer", owner = "right", params = [{consume = true, id = "v1", name = "message", targets = ["second"], type = "queue<u32>"}], result = "void", rule = "transfer"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.method.call @shared::@right.transfer.0.call.1.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:shared", instance = "shared", interface = "Module:shared:add", method = "add", role = "method"} : (!acir.queue<i32>) -> i32
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void", slots = [0]}
    }
    acir.work attributes {acir.function = {kind = "work", name = "right_Work", owner = "right", params = [], result = "void"}} {
      %0 = acir.config "choose_left" {acir.id = "v1", acir.source_type = "bool"} : i1
      %1 = acir.unary %0 "not" {acir.id = "v2", acir.source_type = "bool"} : i1 -> i1
      %2 = acir.ref @top::@second {acir.id = "v4", acir.source_type = "queue<u32>"} : !acir.queue<i32>
      scf.if %1 {
        acir.rule.call @right::@right.transfer(%2) {acir.id = "v5", acir.source_type = "void", output_bindings = [@right::@right.out]} : (!acir.queue<i32>) -> ()
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @independent attributes {acir.instance = {contract = "Module:independent", definition = "src_shared_state_16f7c6b8.Client", inputs = {accumulator = {instance = "separate"}, enabled = {resources = [], type = "bool", value}, source = {resources = ["third"], type = "queue<u32>", value}}, interfaces = {}, members = {accumulator = {instance = "separate"}, enabled = {resources = [], type = "bool", value}, out = {resources = ["independent.out"], type = "queue<u32>", value}, source = {resources = ["third"], type = "queue<u32>", value}}, methods = [], name = "independent", outputs = {resources = ["independent.out"], type = "queue<u32>", value}, parent = "top"}} {
    acir.instance @accumulator = @separate
    acir.queue @independent.out : i32 {capacity_fn = "ac_init_independent.out_cap", initial_fn = "ac_init_independent.out", name = "independent.out", type = "u32"}
    func.func private @ac_init_first_cap(i1) -> i32
    func.func private @ac_init_first(i1) -> !acir.vector<i32>
    func.func private @ac_init_second_cap(i1) -> i32
    func.func private @ac_init_second(i1) -> !acir.vector<i32>
    func.func private @ac_init_third_cap(i1) -> i32
    func.func private @ac_init_third(i1) -> !acir.vector<i32>
    func.func private @ac_init_shared.total(i1) -> i32
    func.func private @ac_init_separate.total(i1) -> i32
    func.func private @ac_init_left.out_cap(i1) -> i32
    func.func private @ac_init_left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_right.out_cap(i1) -> i32
    func.func private @ac_init_right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_independent.out_cap(i1) -> i32
    func.func private @ac_init_independent.out(i1) -> !acir.vector<i32>
    acir.rule @independent.transfer : (!acir.queue<i32>) -> () {acir.function = {kind = "rule", name = "independent.transfer", owner = "independent", params = [{consume = true, id = "v1", name = "message", targets = ["third"], type = "queue<u32>"}], result = "void", rule = "transfer"}, output_types = [!acir.queue<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>):
      %0 = acir.method.call @separate::@independent.transfer.0.call.2.3(%arg0) {acir.id = "v4", acir.source_type = "u32", contract = "Module:separate", instance = "separate", interface = "Module:separate:add", method = "add", role = "method"} : (!acir.queue<i32>) -> i32
      acir.return %0 : i32 {acir.id = "v5", acir.source_type = "void", slots = [0]}
    }
    acir.work attributes {acir.function = {kind = "work", name = "independent_Work", owner = "independent", params = [], result = "void"}} {
      %0 = acir.ref @top::@third {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32>
      acir.rule.call @independent::@independent.transfer(%0) {acir.id = "v4", acir.source_type = "void", output_bindings = [@independent::@independent.out]} : (!acir.queue<i32>) -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_shared_state_16f7c6b8.SharedState", inputs = {choose_left = {resources = [], type = "bool", value}}, interfaces = {}, members = {choose_left = {resources = [], type = "bool", value}, first = {resources = ["first"], type = "queue<u32>", value}, independent = {instance = "independent"}, left = {instance = "left"}, right = {instance = "right"}, second = {resources = ["second"], type = "queue<u32>", value}, separate = {instance = "separate"}, shared = {instance = "shared"}, third = {resources = ["third"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["shared.total"], type = "cell<u32>", value}, {resources = ["separate.total"], type = "cell<u32>", value}, {resources = ["left.out"], type = "queue<u32>", value}, {resources = ["right.out"], type = "queue<u32>", value}, {resources = ["independent.out"], type = "queue<u32>", value}], parent}} {
    acir.instance @shared = @shared
    acir.instance @separate = @separate
    acir.instance @left = @left
    acir.instance @right = @right
    acir.instance @independent = @independent
    acir.queue @first : i32 {capacity_fn = "ac_init_first_cap", initial_fn = "ac_init_first", name = "first", type = "u32"}
    acir.queue @second : i32 {capacity_fn = "ac_init_second_cap", initial_fn = "ac_init_second", name = "second", type = "u32"}
    acir.queue @third : i32 {capacity_fn = "ac_init_third_cap", initial_fn = "ac_init_third", name = "third", type = "u32"}
    func.func private @ac_init_first_cap(i1) -> i32
    func.func private @ac_init_first(i1) -> !acir.vector<i32>
    func.func private @ac_init_second_cap(i1) -> i32
    func.func private @ac_init_second(i1) -> !acir.vector<i32>
    func.func private @ac_init_third_cap(i1) -> i32
    func.func private @ac_init_third(i1) -> !acir.vector<i32>
    func.func private @ac_init_shared.total(i1) -> i32
    func.func private @ac_init_separate.total(i1) -> i32
    func.func private @ac_init_left.out_cap(i1) -> i32
    func.func private @ac_init_left.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_right.out_cap(i1) -> i32
    func.func private @ac_init_right.out(i1) -> !acir.vector<i32>
    func.func private @ac_init_independent.out_cap(i1) -> i32
    func.func private @ac_init_independent.out(i1) -> !acir.vector<i32>
  }
}


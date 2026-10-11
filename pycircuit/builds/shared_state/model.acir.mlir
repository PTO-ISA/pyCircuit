module attributes {acir.ehr = [{aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "first", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "independent.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "left.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "right.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "second", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "third", same_port = "invisible", semantics = "last_effective_prior_port"}], acir.model = {config = [{name = "choose_left", type = "bool"}], constants = {}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:shared", methods = ["add"], relations = []}, {id = "Module:separate", methods = ["add"], relations = []}, {id = "Module:left", methods = [], relations = []}, {id = "Module:right", methods = [], relations = []}, {id = "Module:independent", methods = [], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {first = "first", second = "second", third = "third"}, format_version = 6 : i64, instances = [{contract = "Module:shared", definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, interfaces = {add = "Module:shared:add"}, members = {total = {resources = ["shared.total"], type = "cell<u32>", value}}, methods = ["add"], name = "shared", outputs = {resources = ["shared.total"], type = "cell<u32>", value}, parent = "top"}, {contract = "Module:separate", definition = "src_shared_state_16f7c6b8.Accumulator", inputs = {}, interfaces = {add = "Module:separate:add"}, members = {total = {resources = ["separate.total"], type = "cell<u32>", value}}, methods = ["add"], name = "separate", outputs = {resources = ["separate.total"], type = "cell<u32>", value}, parent = "top"}, {contract = "Module:left", definition = "src_shared_state_16f7c6b8.Client", inputs = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, source = {resources = ["first"], type = "queue<u32>", value}}, interfaces = {}, members = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, out = {resources = ["left.out"], type = "queue<u32>", value}, source = {resources = ["first"], type = "queue<u32>", value}}, methods = [], name = "left", outputs = {resources = ["left.out"], type = "queue<u32>", value}, parent = "top"}, {contract = "Module:right", definition = "src_shared_state_16f7c6b8.Client", inputs = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, source = {resources = ["second"], type = "queue<u32>", value}}, interfaces = {}, members = {accumulator = {instance = "shared"}, enabled = {resources = [], type = "bool", value}, out = {resources = ["right.out"], type = "queue<u32>", value}, source = {resources = ["second"], type = "queue<u32>", value}}, methods = [], name = "right", outputs = {resources = ["right.out"], type = "queue<u32>", value}, parent = "top"}, {contract = "Module:independent", definition = "src_shared_state_16f7c6b8.Client", inputs = {accumulator = {instance = "separate"}, enabled = {resources = [], type = "bool", value}, source = {resources = ["third"], type = "queue<u32>", value}}, interfaces = {}, members = {accumulator = {instance = "separate"}, enabled = {resources = [], type = "bool", value}, out = {resources = ["independent.out"], type = "queue<u32>", value}, source = {resources = ["third"], type = "queue<u32>", value}}, methods = [], name = "independent", outputs = {resources = ["independent.out"], type = "queue<u32>", value}, parent = "top"}, {contract = "Module:top", definition = "src_shared_state_16f7c6b8.SharedState", inputs = {choose_left = {resources = [], type = "bool", value}}, interfaces = {}, members = {choose_left = {resources = [], type = "bool", value}, first = {resources = ["first"], type = "queue<u32>", value}, independent = {instance = "independent"}, left = {instance = "left"}, right = {instance = "right"}, second = {resources = ["second"], type = "queue<u32>", value}, separate = {instance = "separate"}, shared = {instance = "shared"}, third = {resources = ["third"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["shared.total"], type = "cell<u32>", value}, {resources = ["separate.total"], type = "cell<u32>", value}, {resources = ["left.out"], type = "queue<u32>", value}, {resources = ["right.out"], type = "queue<u32>", value}, {resources = ["independent.out"], type = "queue<u32>", value}], parent}], modules = [{name = "left", rules = [{function = "left.transfer", name = "transfer"}], work = "left_Work"}, {name = "right", rules = [{function = "right.transfer", name = "transfer"}], work = "right_Work"}, {name = "independent", rules = [{function = "independent.transfer", name = "transfer"}], work = "independent_Work"}], top = "SharedState", type_order = [], types = {}}} {
  acir.resource "first" "queue" "u32" {capacity_fn = "ac_init_first_cap", contract = "Queue", initial_fn = "ac_init_first", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 32 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "second" "queue" "u32" {capacity_fn = "ac_init_second_cap", contract = "Queue", initial_fn = "ac_init_second", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 33 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "third" "queue" "u32" {capacity_fn = "ac_init_third_cap", contract = "Queue", initial_fn = "ac_init_third", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 34 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "shared.total" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_shared.total", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 7 : i64}, visibility = "old"}
  acir.resource "separate.total" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_separate.total", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 7 : i64}, visibility = "old"}
  acir.resource "left.out" "queue" "u32" {capacity_fn = "ac_init_left.out_cap", contract = "Queue", initial_fn = "ac_init_left.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 19 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "right.out" "queue" "u32" {capacity_fn = "ac_init_right.out_cap", contract = "Queue", initial_fn = "ac_init_right.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 19 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "independent.out" "queue" "u32" {capacity_fn = "ac_init_independent.out_cap", contract = "Queue", initial_fn = "ac_init_independent.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/shared_state.py", line = 19 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  func.func @ac_init_first_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_first_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_first(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_first", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc1)
    %0 = acir.aggregate(%c3_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_second_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_second_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_second(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_second", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32 loc(#loc2)
    %0 = acir.aggregate(%c5_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_third_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_third_cap", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_third(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_third", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc3)
    %0 = acir.aggregate(%c9_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_shared.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_shared.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  func.func @ac_init_separate.total(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_separate.total", params = [{id = "cfg0", name = "choose_left", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
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
  func.func @left.transfer.0.call.0.1(%arg0: !acir.queue<i32>) -> i32 attributes {acir.function = {contract = "Module:shared", definition = "add", instance = "shared", interface = "Module:shared:add", kind = "method", method = "add", name = "left.transfer.0.call.0.1", params = [{consume = true, id = "v1", name = "message", targets = ["first"], type = "queue<u32>"}], result = "u32"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "shared.total" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc5)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc6)
    %2 = acir.read %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc7)
    acir.pop %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.queue<i32> loc(#loc7)
    acir.write %arg0, %false {acir.id = "v7", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "first"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc7)
    %3 = arith.addi %1, %2 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc6)
    acir.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc8)
    %4 = acir.read %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc9)
    acir.pop %arg0 {acir.id = "v11", acir.source_type = "void"} : !acir.queue<i32> loc(#loc9)
    acir.write %arg0, %false {acir.id = "v13", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "first"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc9)
    return %4 : i32 loc(#loc10)
  }
  func.func @right.transfer.0.call.1.1(%arg0: !acir.queue<i32>) -> i32 attributes {acir.function = {contract = "Module:shared", definition = "add", instance = "shared", interface = "Module:shared:add", kind = "method", method = "add", name = "right.transfer.0.call.1.1", params = [{consume = true, id = "v1", name = "message", targets = ["second"], type = "queue<u32>"}], result = "u32"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "shared.total" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc5)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc6)
    %2 = acir.read %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc7)
    acir.pop %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.queue<i32> loc(#loc7)
    acir.write %arg0, %false {acir.id = "v7", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "second"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc7)
    %3 = arith.addi %1, %2 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc6)
    acir.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc8)
    %4 = acir.read %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc9)
    acir.pop %arg0 {acir.id = "v11", acir.source_type = "void"} : !acir.queue<i32> loc(#loc9)
    acir.write %arg0, %false {acir.id = "v13", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "second"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc9)
    return %4 : i32 loc(#loc10)
  }
  func.func @independent.transfer.0.call.2.1(%arg0: !acir.queue<i32>) -> i32 attributes {acir.function = {contract = "Module:separate", definition = "add", instance = "separate", interface = "Module:separate:add", kind = "method", method = "add", name = "independent.transfer.0.call.2.1", params = [{consume = true, id = "v1", name = "message", targets = ["third"], type = "queue<u32>"}], result = "u32"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "separate.total" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc5)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc6)
    %2 = acir.read %arg0 {acir.id = "v4", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc7)
    acir.pop %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.queue<i32> loc(#loc7)
    acir.write %arg0, %false {acir.id = "v7", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "third"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc7)
    %3 = arith.addi %1, %2 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc6)
    acir.write %0, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc8)
    %4 = acir.read %arg0 {acir.id = "v10", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32 loc(#loc9)
    acir.pop %arg0 {acir.id = "v11", acir.source_type = "void"} : !acir.queue<i32> loc(#loc9)
    acir.write %arg0, %false {acir.id = "v13", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "third"}], view = "ehr"} : !acir.queue<i32>, i1 loc(#loc9)
    return %4 : i32 loc(#loc10)
  }
  func.func @left.transfer(%arg0: !acir.queue<i32>) attributes {acir.function = {kind = "rule", name = "left.transfer", outputs = [{kind = "queue", name = "left.out", targets = ["left.out"], type = "u32"}], owner = "left", params = [{consume = true, id = "v1", name = "message", targets = ["first"], type = "queue<u32>"}], result = "void", rule = "transfer"}} {
    %0 = call @left.transfer.0.call.0.1(%arg0) {acir.id = "v2", acir.source_type = "u32", contract = "Module:shared", instance = "shared", interface = "Module:shared:add", method = "add", role = "method"} : (!acir.queue<i32>) -> i32 loc(#loc11)
    %1 = acir.get "left.out" {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc12)
    %2 = acir.read %1 {acir.id = "v4", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "left.out"}], view = "ehr"} : !acir.queue<i32> -> i1 loc(#loc13)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc13)
    acir.guard %3 "Queue.push capacity" {acir.id = "v6", acir.source_type = "void", implicit = true} loc(#loc13)
    acir.push %1, %0 {acir.id = "v7", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc13)
    return loc(#loc13)
  }
  func.func @left_Work() attributes {acir.function = {kind = "work", name = "left_Work", owner = "left", params = [], result = "void"}} {
    %0 = acir.config "choose_left" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc14)
    %1 = acir.get "first" {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc15)
    cf.cond_br %0, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "transfer"(%1) {acir.id = "v4", acir.source_type = "void", callee = "left.transfer"} : (!acir.queue<i32>) -> () loc(#loc16)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @right.transfer(%arg0: !acir.queue<i32>) attributes {acir.function = {kind = "rule", name = "right.transfer", outputs = [{kind = "queue", name = "right.out", targets = ["right.out"], type = "u32"}], owner = "right", params = [{consume = true, id = "v1", name = "message", targets = ["second"], type = "queue<u32>"}], result = "void", rule = "transfer"}} {
    %0 = call @right.transfer.0.call.1.1(%arg0) {acir.id = "v2", acir.source_type = "u32", contract = "Module:shared", instance = "shared", interface = "Module:shared:add", method = "add", role = "method"} : (!acir.queue<i32>) -> i32 loc(#loc11)
    %1 = acir.get "right.out" {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc12)
    %2 = acir.read %1 {acir.id = "v4", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "right.out"}], view = "ehr"} : !acir.queue<i32> -> i1 loc(#loc13)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc13)
    acir.guard %3 "Queue.push capacity" {acir.id = "v6", acir.source_type = "void", implicit = true} loc(#loc13)
    acir.push %1, %0 {acir.id = "v7", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc13)
    return loc(#loc13)
  }
  func.func @right_Work() attributes {acir.function = {kind = "work", name = "right_Work", owner = "right", params = [], result = "void"}} {
    %0 = acir.config "choose_left" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc14)
    %1 = acir.unary %0 "not" {acir.id = "v2", acir.source_type = "bool"} : i1 -> i1 loc(#loc17)
    %2 = acir.get "second" {acir.id = "v4", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc18)
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "transfer"(%2) {acir.id = "v5", acir.source_type = "void", callee = "right.transfer"} : (!acir.queue<i32>) -> () loc(#loc16)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @independent.transfer(%arg0: !acir.queue<i32>) attributes {acir.function = {kind = "rule", name = "independent.transfer", outputs = [{kind = "queue", name = "independent.out", targets = ["independent.out"], type = "u32"}], owner = "independent", params = [{consume = true, id = "v1", name = "message", targets = ["third"], type = "queue<u32>"}], result = "void", rule = "transfer"}} {
    %0 = call @independent.transfer.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "u32", contract = "Module:separate", instance = "separate", interface = "Module:separate:add", method = "add", role = "method"} : (!acir.queue<i32>) -> i32 loc(#loc11)
    %1 = acir.get "independent.out" {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc12)
    %2 = acir.read %1 {acir.id = "v4", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "independent.out"}], view = "ehr"} : !acir.queue<i32> -> i1 loc(#loc13)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc13)
    acir.guard %3 "Queue.push capacity" {acir.id = "v6", acir.source_type = "void", implicit = true} loc(#loc13)
    acir.push %1, %0 {acir.id = "v7", acir.source_type = "void"} : !acir.queue<i32>, i32 loc(#loc13)
    return loc(#loc13)
  }
  func.func @independent_Work() attributes {acir.function = {kind = "work", name = "independent_Work", owner = "independent", params = [], result = "void"}} {
    %0 = acir.get "third" {acir.id = "v3", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc19)
    acir.invoke "transfer"(%0) {acir.id = "v4", acir.source_type = "void", callee = "independent.transfer"} : (!acir.queue<i32>) -> () loc(#loc16)
    return
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":32:38)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":33:39)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":34:38)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":7:37)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":7:13)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":11:23)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":11:37)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":11:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:16)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":23:16)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":19:11)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":23:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":31:17)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":32:13)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":26:15)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":38:36)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":33:14)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":34:13)


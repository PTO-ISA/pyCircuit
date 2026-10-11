module attributes {acir.ehr = [{aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "router.accepted", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "source", same_port = "invisible", semantics = "last_effective_prior_port"}], acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:ledger", methods = ["record"], relations = []}, {id = "Module:router", methods = [], relations = []}, {id = "Module:observer", methods = [], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {source = "source"}, format_version = 6 : i64, instances = [{contract = "Module:ledger", definition = "src_mixed_ports_9bff12ad.Ledger", inputs = {}, interfaces = {record = "Module:ledger:record"}, members = {observed = {resources = ["ledger.observed"], type = "cell<u32>", value}}, methods = ["record"], name = "ledger", outputs = {resources = ["ledger.observed"], type = "cell<u32>", value}, parent = "top"}, {contract = "Module:router", definition = "src_mixed_ports_9bff12ad.Router", inputs = {enabled = {resources = [], type = "bool", value}, ledger = {instance = "ledger"}, source = {resources = ["source"], type = "queue<u32>", value}}, interfaces = {}, members = {accepted = {resources = ["router.accepted"], type = "queue<u32>", value}, enabled = {resources = [], type = "bool", value}, last_accepted = {resources = ["router.last_accepted"], type = "cell<u32>", value}, ledger = {instance = "ledger"}, result = {resources = ["router.result"], type = "signal<u32>", value}, source = {resources = ["source"], type = "queue<u32>", value}}, methods = [], name = "router", outputs = [{resources = ["router.accepted"], type = "queue<u32>", value}, {resources = ["router.last_accepted"], type = "cell<u32>", value}, {resources = ["router.result"], type = "signal<u32>", value}], parent = "top"}, {contract = "Module:observer", definition = "src_mixed_ports_9bff12ad.Observer", inputs = {value = {resources = ["router.result"], type = "signal<u32>", value}}, interfaces = {}, members = {last = {resources = ["observer.last"], type = "cell<u32>", value}, value = {resources = ["router.result"], type = "signal<u32>", value}}, methods = [], name = "observer", outputs = {resources = ["observer.last"], type = "cell<u32>", value}, parent = "top"}, {contract = "Module:top", definition = "src_mixed_ports_9bff12ad.MixedPorts", inputs = {enabled = {resources = [], type = "bool", value}}, interfaces = {}, members = {enabled = {resources = [], type = "bool", value}, ledger = {instance = "ledger"}, observer = {instance = "observer"}, router = {instance = "router"}, source = {resources = ["source"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["router.accepted"], type = "queue<u32>", value}, {resources = ["router.last_accepted"], type = "cell<u32>", value}, {resources = ["observer.last"], type = "cell<u32>", value}], parent}], modules = [{name = "router", rules = [{function = "router.route", name = "route"}], work = "router_Work"}, {name = "observer", rules = [{function = "observer.sample", name = "sample"}], work = "observer_Work"}], top = "MixedPorts", type_order = [], types = {}}} {
  acir.resource "source" "queue" "u32" {capacity_fn = "ac_init_source_cap", contract = "Queue", initial_fn = "ac_init_source", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/mixed_ports.py", line = 52 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "ledger.observed" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_ledger.observed", loc = {column = 15 : i64, file = "/home/lc/tmp/pycircuit/examples/mixed_ports.py", line = 7 : i64}, visibility = "old"}
  acir.resource "router.accepted" "queue" "u32" {capacity_fn = "ac_init_router.accepted_cap", contract = "Queue", initial_fn = "ac_init_router.accepted", loc = {column = 15 : i64, file = "/home/lc/tmp/pycircuit/examples/mixed_ports.py", line = 31 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "router.last_accepted" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_router.last_accepted", loc = {column = 20 : i64, file = "/home/lc/tmp/pycircuit/examples/mixed_ports.py", line = 32 : i64}, visibility = "old"}
  acir.resource "router.result" "signal" "u32" {contract = "Signal", initial_fn = "ac_init_router.result", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/mixed_ports.py", line = 33 : i64}, visibility = "current"}
  acir.resource "observer.last" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_observer.last", loc = {column = 11 : i64, file = "/home/lc/tmp/pycircuit/examples/mixed_ports.py", line = 18 : i64}, visibility = "old"}
  func.func @ac_init_source_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_source_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_source(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_source", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %0 = acir.aggregate(%c7_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_ledger.observed(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_ledger.observed", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_router.accepted_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_router.accepted_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    return %c2_i32 : i32
  }
  func.func @ac_init_router.accepted(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_router.accepted", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_router.last_accepted(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_router.last_accepted", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_router.result(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_router.result", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_observer.last(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_observer.last", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @router.route.0.call.0.6(%arg0: i32) attributes {acir.function = {contract = "Module:ledger", definition = "record", instance = "ledger", interface = "Module:ledger:record", kind = "method", method = "record", name = "router.route.0.call.0.6", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %0 = acir.get "ledger.observed" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %0, %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @ac_summary_ledger.record.0.call.1.1(%arg0: i32) attributes {acir.function = {contract = "Module:ledger", definition = "record", instance = "ledger", interface = "Module:ledger:record", kind = "method", method = "record", name = "ac_summary_ledger.record.0.call.1.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %0 = acir.get "ledger.observed" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %0, %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @router.route(%arg0: !acir.queue<i32>, %arg1: i1) attributes {acir.function = {kind = "rule", name = "router.route", outputs = [{kind = "queue", name = "router.accepted", targets = ["router.accepted"], type = "u32"}, {kind = "cell", name = "router.last_accepted", targets = ["router.last_accepted"], type = "u32"}, {kind = "signal", name = "router.result", targets = ["router.result"], type = "u32"}], owner = "router", params = [{consume = true, id = "v1", name = "message", targets = ["source"], type = "queue<u32>"}, {consume = false, id = "v2", name = "take", targets = [], type = "bool"}], result = "void", rule = "route"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.read %arg0 {acir.id = "v3", acir.source_type = "u32", role = "consume", view = "old"} : !acir.queue<i32> -> i32
    acir.pop %arg0 {acir.id = "v4", acir.source_type = "void"} : !acir.queue<i32>
    acir.write %arg0, %false {acir.id = "v6", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "source"}], view = "ehr"} : !acir.queue<i32>, i1
    call @router.route.0.call.0.6(%0) {acir.id = "v7", acir.source_type = "void", contract = "Module:ledger", instance = "ledger", interface = "Module:ledger:record", method = "record", role = "method"} : (i32) -> ()
    %1 = acir.get "router.accepted" {acir.id = "v11", acir.source_type = "queue<u32>"} : !acir.queue<i32>
    %2 = acir.get "router.last_accepted" {acir.id = "v16", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %3 = acir.get "router.result" {acir.id = "v18", acir.source_type = "signal<u32>"} : !acir.signal<i32>
    cf.cond_br %arg1, ^bb1, ^bb2(%0 : i32)
  ^bb1:  // pred: ^bb0
    %4 = arith.addi %0, %c1_i32 {acir.id = "v10", acir.source_type = "u32"} : i32
    %5 = acir.read %1 {acir.id = "v12", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "router.accepted"}], view = "ehr"} : !acir.queue<i32> -> i1
    %6 = acir.unary %5 "not" {acir.id = "v13", acir.source_type = "bool"} : i1 -> i1
    acir.guard %6 "Queue.push capacity" {acir.id = "v14", acir.source_type = "void", implicit = true}
    acir.push %1, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.queue<i32>, i32
    acir.write %2, %0 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<i32>, i32
    acir.write %3, %0 {acir.id = "v19", acir.source_type = "void", interface = "set", view = "current"} : !acir.signal<i32>, i32
    return
  ^bb2(%7: i32):  // pred: ^bb0
    acir.write %3, %7 {acir.id = "v25", acir.source_type = "void", interface = "set", view = "current"} : !acir.signal<i32>, i32
    return
  }
  func.func @router_Work() attributes {acir.function = {kind = "work", name = "router_Work", owner = "router", params = [], result = "void"}} {
    %0 = acir.get "source" {acir.id = "v1", acir.source_type = "queue<u32>"} : !acir.queue<i32>
    %1 = acir.config "enabled" {acir.id = "v2", acir.source_type = "bool"} : i1
    acir.invoke "route"(%0, %1) {acir.id = "v3", acir.source_type = "void", callee = "router.route"} : (!acir.queue<i32>, i1) -> ()
    return
  }
  func.func @observer.sample(%arg0: !acir.signal<i32>) attributes {acir.function = {kind = "rule", name = "observer.sample", outputs = [], owner = "observer", params = [{consume = false, id = "v1", name = "x", targets = ["router.result"], type = "signal<u32>"}], result = "void", rule = "sample"}} {
    %0 = acir.get "observer.last" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %arg0 {acir.id = "v3", acir.source_type = "u32", interface = "get", role = "observe", view = "current"} : !acir.signal<i32> -> i32
    acir.write %0, %1 {acir.id = "v4", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @observer_Work() attributes {acir.function = {kind = "work", name = "observer_Work", owner = "observer", params = [], result = "void"}} {
    %0 = acir.get "router.result" {acir.id = "v1", acir.source_type = "signal<u32>"} : !acir.signal<i32>
    acir.invoke "sample"(%0) {acir.id = "v2", acir.source_type = "void", callee = "observer.sample"} : (!acir.signal<i32>) -> ()
    return
  }
}


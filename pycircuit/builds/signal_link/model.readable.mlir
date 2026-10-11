module attributes {acir.ehr = [], acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:producer", methods = ["configure", "peek"], relations = []}, {id = "Module:reader", methods = [], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {}, format_version = 6 : i64, instances = [{contract = "Module:producer", definition = "src_signal_link_6cc8a7a9.Producer", inputs = {enabled = {resources = [], type = "bool", value}}, interfaces = {configure = "Module:producer:configure", peek = "Module:producer:peek"}, members = {count = {resources = ["producer.count"], type = "cell<u32>", value}, enabled = {resources = [], type = "bool", value}, offset = {resources = ["producer.offset"], type = "cell<u32>", value}, result = {resources = ["producer.result"], type = "signal<u32>", value}}, methods = ["configure", "peek"], name = "producer", outputs = {resources = ["producer.result"], type = "signal<u32>", value}, parent = "top"}, {contract = "Module:reader", definition = "src_signal_link_6cc8a7a9.Reader", inputs = {source = {resources = ["producer.result"], type = "signal<u32>", value}}, interfaces = {}, members = {last = {resources = ["reader.last"], type = "cell<u32>", value}, source = {resources = ["producer.result"], type = "signal<u32>", value}}, methods = [], name = "reader", outputs = {resources = ["reader.last"], type = "cell<u32>", value}, parent = "top"}, {contract = "Module:top", definition = "src_signal_link_6cc8a7a9.SignalLink", inputs = {enabled = {resources = [], type = "bool", value}}, interfaces = {}, members = {enabled = {resources = [], type = "bool", value}, producer = {instance = "producer"}, reader = {instance = "reader"}}, methods = [], name = "top", outputs = [{resources = ["producer.result"], type = "signal<u32>", value}, {resources = ["reader.last"], type = "cell<u32>", value}], parent}], modules = [{name = "producer", rules = [{function = "producer.advance", name = "advance"}], work = "producer_Work"}, {name = "reader", rules = [{function = "reader.observe", name = "observe"}], work = "reader_Work"}, {name = "top", rules = [{function = "top.configure", name = "configure"}], work = "top_Work"}], top = "SignalLink", type_order = [], types = {}}} {
  acir.resource "producer.offset" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_producer.offset", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/signal_link.py", line = 7 : i64}, visibility = "old"}
  acir.resource "producer.count" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_producer.count", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/signal_link.py", line = 8 : i64}, visibility = "old"}
  acir.resource "producer.result" "signal" "u32" {contract = "Signal", initial_fn = "ac_init_producer.result", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/signal_link.py", line = 9 : i64}, visibility = "current"}
  acir.resource "reader.last" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_reader.last", loc = {column = 11 : i64, file = "/home/lc/tmp/pycircuit/examples/signal_link.py", line = 31 : i64}, visibility = "old"}
  func.func @ac_init_producer.offset(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_producer.offset", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_producer.count(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_producer.count", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_producer.result(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_producer.result", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_reader.last(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_reader.last", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @top.configure.0.call.0.1(%arg0: i32) attributes {acir.function = {contract = "Module:producer", definition = "configure", instance = "producer", interface = "Module:producer:configure", kind = "method", method = "configure", name = "top.configure.0.call.0.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %0 = acir.get "producer.offset" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %0, %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @ac_summary_producer.configure.0.call.1.1(%arg0: i32) attributes {acir.function = {contract = "Module:producer", definition = "configure", instance = "producer", interface = "Module:producer:configure", kind = "method", method = "configure", name = "ac_summary_producer.configure.0.call.1.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %0 = acir.get "producer.offset" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %0, %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @ac_summary_producer.peek.0.call.2.0() -> i32 attributes {acir.function = {contract = "Module:producer", definition = "peek", instance = "producer", interface = "Module:producer:peek", kind = "method", method = "peek", name = "ac_summary_producer.peek.0.call.2.0", params = [], result = "u32", summary = true}} {
    %0 = acir.get "producer.result" {acir.id = "v1", acir.source_type = "signal<u32>"} : !acir.signal<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", interface = "get", role = "observe", view = "current"} : !acir.signal<i32> -> i32
    return %1 : i32
  }
  func.func @producer.advance() attributes {acir.function = {kind = "rule", name = "producer.advance", outputs = [], owner = "producer", params = [], result = "void", rule = "advance"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "producer.count" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = acir.get "producer.offset" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %3 = acir.read %2 {acir.id = "v4", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %4 = arith.addi %1, %3 {acir.id = "v5", acir.source_type = "u32"} : i32
    %5 = acir.get "producer.result" {acir.id = "v6", acir.source_type = "signal<u32>"} : !acir.signal<i32>
    acir.write %5, %4 {acir.id = "v7", acir.source_type = "void", interface = "set", view = "current"} : !acir.signal<i32>, i32
    %6 = acir.read %0 {acir.id = "v8", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %7 = arith.addi %6, %c1_i32 {acir.id = "v10", acir.source_type = "u32"} : i32
    acir.write %0, %7 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @producer_Work() attributes {acir.function = {kind = "work", name = "producer_Work", owner = "producer", params = [], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
    %0 = acir.config "enabled" {acir.id = "v1", acir.source_type = "bool"} : i1
    %1 = acir.get "producer.count" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %2 = acir.read %1 {acir.id = "v4", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %3 = arith.cmpi ult, %2, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    acir.invoke "advance"() {acir.id = "v12", acir.source_type = "void", callee = "producer.advance"} : () -> ()
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @reader.observe(%arg0: i32, %arg1: i1) attributes {acir.function = {kind = "rule", name = "reader.observe", outputs = [], owner = "reader", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}, {consume = false, id = "v2", name = "$guard_observe", targets = [], type = "bool"}], result = "void", rule = "observe"}} {
    acir.guard %arg1 "ready" {acir.id = "v3", acir.source_type = "void", source = "variable"}
    %0 = acir.get "reader.last" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @reader_Work() attributes {acir.function = {kind = "work", name = "reader_Work", owner = "reader", params = [], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "producer.result" {acir.id = "v1", acir.source_type = "signal<u32>"} : !acir.signal<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", interface = "get", role = "observe", view = "current"} : !acir.signal<i32> -> i32
    %2 = arith.addi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    acir.invoke "observe"(%2, %3) {acir.id = "v7", acir.source_type = "void", callee = "reader.observe"} : (i32, i1) -> ()
    return
  }
  func.func @top.configure() attributes {acir.function = {kind = "rule", name = "top.configure", outputs = [], owner = "top", params = [], result = "void", rule = "configure"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    call @top.configure.0.call.0.1(%c4_i32) {acir.id = "v2", acir.source_type = "void", contract = "Module:producer", instance = "producer", interface = "Module:producer:configure", method = "configure", role = "method"} : (i32) -> ()
    return
  }
  func.func @top_Work() attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
    acir.invoke "configure"() {acir.id = "v1", acir.source_type = "void", callee = "top.configure"} : () -> ()
    return
  }
}


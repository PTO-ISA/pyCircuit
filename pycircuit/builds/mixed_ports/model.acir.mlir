module attributes {acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {source = "source"}, format_version = 7 : i64, top = "MixedPorts", type_order = [], types = {}}} {
  func.func @ac_init_source_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_source_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_source(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_source", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc1)
    %0 = acir.aggregate(%c7_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_ledger.observed(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_ledger.observed", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc2)
    return %c0_i32 : i32
  }
  func.func @ac_init_router.accepted_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_router.accepted_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc3)
    return %c2_i32 : i32
  }
  func.func @ac_init_router.accepted(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_router.accepted", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_router.last_accepted(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_router.last_accepted", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  func.func @ac_init_router.result(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_router.result", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc5)
    return %c0_i32 : i32
  }
  func.func @ac_init_observer.last(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_observer.last", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc6)
    return %c0_i32 : i32
  }
  acir.module @ledger attributes {acir.instance = {contract = "Module:ledger", definition = "src_mixed_ports_9bff12ad.Ledger", inputs = {}, interfaces = {record = "Module:ledger:record"}, members = {observed = {resources = ["ledger.observed"], type = "cell<u32>", value}}, methods = ["record"], name = "ledger", outputs = {resources = ["ledger.observed"], type = "cell<u32>", value}, parent = "top"}} {
    acir.interface @record {implementations = [@ledger::@router.route.0.call.0.5, @ledger::@ac_summary_ledger.record.0.call.1.1], orders = []}
    acir.cell @ledger.observed : i32 {initial_fn = "ac_init_ledger.observed", name = "ledger.observed", type = "u32"}
    func.func private @ac_init_source_cap(i1) -> i32
    func.func private @ac_init_source(i1) -> !acir.vector<i32>
    func.func private @ac_init_ledger.observed(i1) -> i32
    func.func private @ac_init_router.accepted_cap(i1) -> i32
    func.func private @ac_init_router.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_router.last_accepted(i1) -> i32
    func.func private @ac_init_router.result(i1) -> i32
    func.func private @ac_init_observer.last(i1) -> i32
    acir.method @router.route.0.call.0.5 : (i32) -> () {acir.function = {contract = "Module:ledger", definition = "record", instance = "ledger", interface = "Module:ledger:record", kind = "method", method = "record", name = "router.route.0.call.0.5", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @ledger::@record} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @ledger::@ledger.observed {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc7)
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc8)
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_ledger.record.0.call.1.1 : (i32) -> () {acir.function = {contract = "Module:ledger", definition = "record", instance = "ledger", interface = "Module:ledger:record", kind = "method", method = "record", name = "ac_summary_ledger.record.0.call.1.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @ledger::@record} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @ledger::@ledger.observed {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc7)
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc8)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @router attributes {acir.instance = {contract = "Module:router", definition = "src_mixed_ports_9bff12ad.Router", inputs = {enabled = {resources = [], type = "bool", value}, ledger = {instance = "ledger"}, source = {resources = ["source"], type = "queue<u32>", value}}, interfaces = {}, members = {accepted = {resources = ["router.accepted"], type = "queue<u32>", value}, enabled = {resources = [], type = "bool", value}, last_accepted = {resources = ["router.last_accepted"], type = "cell<u32>", value}, ledger = {instance = "ledger"}, result = {resources = ["router.result"], type = "signal<u32>", value}, source = {resources = ["source"], type = "queue<u32>", value}}, methods = [], name = "router", outputs = [{resources = ["router.accepted"], type = "queue<u32>", value}, {resources = ["router.last_accepted"], type = "cell<u32>", value}, {resources = ["router.result"], type = "signal<u32>", value}], parent = "top"}} {
    acir.instance @ledger = @ledger
    acir.queue @router.accepted : i32 {capacity_fn = "ac_init_router.accepted_cap", initial_fn = "ac_init_router.accepted", name = "router.accepted", type = "u32"}
    acir.cell @router.last_accepted : i32 {initial_fn = "ac_init_router.last_accepted", name = "router.last_accepted", type = "u32"}
    acir.signal @router.result : i32 {initial_fn = "ac_init_router.result", name = "router.result", type = "u32"}
    func.func private @ac_init_source_cap(i1) -> i32
    func.func private @ac_init_source(i1) -> !acir.vector<i32>
    func.func private @ac_init_ledger.observed(i1) -> i32
    func.func private @ac_init_router.accepted_cap(i1) -> i32
    func.func private @ac_init_router.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_router.last_accepted(i1) -> i32
    func.func private @ac_init_router.result(i1) -> i32
    func.func private @ac_init_observer.last(i1) -> i32
    acir.rule @router.route : (!acir.queue<i32>, i1) -> () {acir.function = {kind = "rule", name = "router.route", owner = "router", params = [{consume = true, id = "v1", name = "message", targets = ["source"], type = "queue<u32>"}, {consume = false, id = "v2", name = "take", targets = [], type = "bool"}], result = "void", rule = "route"}, output_types = [!acir.queue<i32>, !acir.cell<i32>, !acir.signal<i32>]} body {
    ^bb0(%arg0: !acir.queue<i32>, %arg1: i1):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc9)
      acir.method.call @ledger::@router.route.0.call.0.5(%0) {acir.id = "v6", acir.source_type = "void", contract = "Module:ledger", instance = "ledger", interface = "Module:ledger:record", method = "record", role = "method"} : (i32) -> () loc(#loc10)
      cf.cond_br %arg1, ^bb1, ^bb2(%0 : i32)
    ^bb1:  // pred: ^bb0
      %1 = arith.addi %0, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc11)
      acir.return %1, %0, %0 : i32, i32, i32 {acir.id = "v10", acir.source_type = "void", slots = [0, 1, 2]} loc(#loc12)
    ^bb2(%2: i32):  // pred: ^bb0
      acir.return %2 : i32 {acir.id = "v15", acir.source_type = "void", slots = [2]} loc(#loc13)
    }
    acir.work attributes {acir.function = {kind = "work", name = "router_Work", owner = "router", params = [], result = "void"}} {
      %0 = acir.ref @top::@source {acir.id = "v1", acir.source_type = "queue<u32>"} : !acir.queue<i32> loc(#loc14)
      %1 = acir.config "enabled" {acir.id = "v2", acir.source_type = "bool"} : i1 loc(#loc15)
      acir.rule.call @router::@router.route(%0, %1) {acir.id = "v3", acir.source_type = "void", output_bindings = [@router::@router.accepted, @router::@router.last_accepted, @router::@router.result]} : (!acir.queue<i32>, i1) -> () loc(#loc16)
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @observer attributes {acir.instance = {contract = "Module:observer", definition = "src_mixed_ports_9bff12ad.Observer", inputs = {value = {resources = ["router.result"], type = "signal<u32>", value}}, interfaces = {}, members = {last = {resources = ["observer.last"], type = "cell<u32>", value}, value = {resources = ["router.result"], type = "signal<u32>", value}}, methods = [], name = "observer", outputs = {resources = ["observer.last"], type = "cell<u32>", value}, parent = "top"}} {
    acir.cell @observer.last : i32 {initial_fn = "ac_init_observer.last", name = "observer.last", type = "u32"}
    func.func private @ac_init_source_cap(i1) -> i32
    func.func private @ac_init_source(i1) -> !acir.vector<i32>
    func.func private @ac_init_ledger.observed(i1) -> i32
    func.func private @ac_init_router.accepted_cap(i1) -> i32
    func.func private @ac_init_router.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_router.last_accepted(i1) -> i32
    func.func private @ac_init_router.result(i1) -> i32
    func.func private @ac_init_observer.last(i1) -> i32
    acir.rule @observer.sample : (!acir.signal<i32>) -> () {acir.function = {kind = "rule", name = "observer.sample", owner = "observer", params = [{consume = false, id = "v1", name = "x", targets = ["router.result"], type = "signal<u32>"}], result = "void", rule = "sample"}, output_types = []} body {
    ^bb0(%arg0: !acir.signal<i32>):
      %0 = acir.ref @observer::@observer.last {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc17)
      %1 = acir.signal.get %arg0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32 loc(#loc18)
      acir.cell.write %0, %1 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc19)
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "observer_Work", owner = "observer", params = [], result = "void"}} {
      %0 = acir.ref @router::@router.result {acir.id = "v1", acir.source_type = "signal<u32>"} : !acir.signal<i32> loc(#loc20)
      acir.rule.call @observer::@observer.sample(%0) {acir.id = "v2", acir.source_type = "void", output_bindings = []} : (!acir.signal<i32>) -> () loc(#loc21)
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_mixed_ports_9bff12ad.MixedPorts", inputs = {enabled = {resources = [], type = "bool", value}}, interfaces = {}, members = {enabled = {resources = [], type = "bool", value}, ledger = {instance = "ledger"}, observer = {instance = "observer"}, router = {instance = "router"}, source = {resources = ["source"], type = "queue<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["router.accepted"], type = "queue<u32>", value}, {resources = ["router.last_accepted"], type = "cell<u32>", value}, {resources = ["observer.last"], type = "cell<u32>", value}], parent}} {
    acir.instance @ledger = @ledger
    acir.instance @router = @router
    acir.instance @observer = @observer
    acir.queue @source : i32 {capacity_fn = "ac_init_source_cap", initial_fn = "ac_init_source", name = "source", type = "u32"}
    func.func private @ac_init_source_cap(i1) -> i32
    func.func private @ac_init_source(i1) -> !acir.vector<i32>
    func.func private @ac_init_ledger.observed(i1) -> i32
    func.func private @ac_init_router.accepted_cap(i1) -> i32
    func.func private @ac_init_router.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_router.last_accepted(i1) -> i32
    func.func private @ac_init_router.result(i1) -> i32
    func.func private @ac_init_observer.last(i1) -> i32
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":52:39)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":7:40)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":31:42)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":32:45)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":33:40)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":18:36)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":7:16)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":11:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":39:17)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":40:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":42:20)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":42:13)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":44:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":52:14)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":51:16)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":46:39)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":18:12)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:22)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:9)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":47:37)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":25:5)

